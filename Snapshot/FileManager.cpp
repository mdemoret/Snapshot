#include "FileManager.hpp"
#include <ion/base/stringutils.h>
#include <ion/base/vectordatacontainer.h>
#include <ion/math/matrixutils.h>
#include <ion/math/vectorutils.h>
#include <future>
#include "Hud.hpp"
#include "ion/base/serialize.h"
#include "FinalAction.hpp"
#include "ion/base/settingmanager.h"
#include "Macros.h"

using namespace ion::math;
using namespace std;

namespace Snapshot{

Range3d CalcStatesRange(const std::vector<State>& states)
{
   Range3d bounds(Point3d::Fill(std::numeric_limits<double>::max()), Point3d::Fill(std::numeric_limits<double>::min()));

   for (size_t i = 0; i < states.size(); i++)
   {
      bounds.ExtendByPoint(states[i].Pos);
   }

   return bounds;
}

std::vector<ion::math::Matrix3d> CalcStatesVnb(const std::vector<State>& states)
{
   std::vector<ion::math::Matrix3d> vnbs;

   for (size_t i = 0; i < states.size(); i++)
   {
      vnbs.push_back(SnapshotData::CalcVNB(states[i]));
   }

   return vnbs;
}

std::vector<ion::math::Point3d> CalcStatesPos(const std::vector<State>& states)
{
   std::vector<ion::math::Point3d> pos;

   for (size_t i = 0; i < states.size(); i++)
   {
      pos.push_back(states[i].Pos);
   }

   return pos;
}

SnapshotDataStats::SnapshotDataStats():
   Count(0),
   Pc(0) {}

SnapshotData::SnapshotData(double epoch, const std::vector<State>& stateA, const std::vector<State>& stateB):
   m_Epoch(epoch),
   m_StateAVnb(CalcStatesVnb(stateA)),
   m_StateAPos(CalcStatesPos(stateA)),
   m_StateBPos(CalcStatesPos(stateB)),
   m_Stats() {}

SnapshotData::~SnapshotData() {}

double SnapshotData::GetEpoch() const
{
   return m_Epoch;
}

const SnapshotDataStats& SnapshotData::GetStats() const
{
   return m_Stats;
}

const std::vector<StateVertex>& SnapshotData::GetOutputData() const
{
   return m_OutputData;
}

SnapshotUnitOfWork::SnapshotUnitOfWork(SnapshotData& snapshotData, bool allToAll, float hbr, ProgressHandler& progressHandler, std::atomic<uint32_t>& cancellationToken):
   m_SnapshotData(snapshotData),
   m_AllToAll(allToAll),
   m_HBR(hbr),
   m_ProgressHandler(progressHandler),
   m_CancellationToken(cancellationToken) {}

void SnapshotUnitOfWork::CalcDiffs() const
{
   if (m_AllToAll)
   {
      m_SnapshotData.m_Stats = CalcStats(GetAllToAllData());
   }
   else
   {
      m_SnapshotData.m_Stats = CalcStats(GetOneToOneData());
   }
}

void SnapshotUnitOfWork::CalcOutputs() const
{
   if (m_AllToAll)
   {
      m_SnapshotData.m_OutputData = GenerateVertices(GetAllToAllData());
   }
   else
   {
      m_SnapshotData.m_OutputData = GenerateVertices(GetOneToOneData());
   }
}

SnapshotData& SnapshotUnitOfWork::GetSnapshotData() const
{
   return m_SnapshotData;
}

const std::vector<DiffPoint>& SnapshotUnitOfWork::GetOneToOneData() const
{
   if (m_SnapshotData.m_OneToOneDiffs.size() == 0)
   {
      size_t i = 0;
      size_t aCount = m_SnapshotData.m_StateAVnb.size();

      m_ProgressHandler.SetProgressFunc([&]()
                                        {
                                           return "Calculating One-to-One\n" + ion::base::ValueToString(round(i / static_cast<double>(aCount) * 100.0)) + "% Complete";
                                        });

      for (i = 0; i < aCount; i++)
      {
         Point3d diff = Point3d::ToPoint(m_SnapshotData.m_StateAVnb[i] * (m_SnapshotData.m_StateBPos[i] - m_SnapshotData.m_StateAPos[i]));

         m_SnapshotData.m_OneToOneDiffs.push_back(DiffPoint(Point3f(diff), 1));
      }
   }

   m_ProgressHandler.SetProgressFunc(nullptr);

   return m_SnapshotData.m_OneToOneDiffs;
}

const std::vector<DiffPoint>& SnapshotUnitOfWork::GetAllToAllData() const
{
   if (m_SnapshotData.m_AllToAllDiffs.size() == 0)
   {
      //Get the diff data. This is cached if possible
      auto oneToOneData = GetOneToOneData();

      auto oneToOneStats = CalcStats(oneToOneData);

      auto binCount = static_cast<ion::base::Setting<uint32_t>*>(ion::base::SettingManager::GetSetting(SETTINGS_CONFIG_ALLTOALL_BIN_COUNT))->GetValue();
      auto multiplier = static_cast<ion::base::Setting<float>*>(ion::base::SettingManager::GetSetting(SETTINGS_CONFIG_ALLTOALL_INITIAL_BIN_MULT))->GetValue();
      auto maxTries = static_cast<ion::base::Setting<uint32_t>*>(ion::base::SettingManager::GetSetting(SETTINGS_CONFIG_ALLTOALL_MAX_BIN_TRIES))->GetValue();

      for (size_t i = 1; i <= maxTries; i++)
      {
         try
         {
            TryBinAllToAllData(oneToOneStats.Bounds, multiplier * i, binCount);

            break;
         } catch (std::range_error&)
         {
            LOG(WARNING) << "All-to-All out of bounds. Retry #: " << i;
         }
      }

      if (m_SnapshotData.m_AllToAllDiffs.size() == 0)
         throw std::runtime_error("Count not generate All-to-All with given parameters. Consider increasing the multiplier and max tries");
   }

   m_ProgressHandler.SetProgressFunc(nullptr);

   return m_SnapshotData.m_AllToAllDiffs;
}

void SnapshotUnitOfWork::TryBinAllToAllData(const ion::math::Range3f& bounds, float multiplier, uint32_t binCount) const
{
   //Use the one to one bounds to make a guess on the max size by doing 2X the one to one
   const Point3f& upper = bounds.GetCenter() + multiplier * bounds.GetSize();
   const Point3f& lower = bounds.GetCenter() - multiplier * bounds.GetSize();

   Vector3f multBounds = upper - lower;

   float stepSize = pow((multBounds[0] * multBounds[1] * multBounds[2]) / static_cast<float>(binCount), 1.0f / 3.0f);

   //Now determine the number of steps in each direction
   Vector3f stepCount = multBounds / stepSize;

   size_t xCount = static_cast<uint32_t>(ceil(abs(stepCount[0]))) + 1;
   size_t yCount = static_cast<uint32_t>(ceil(abs(stepCount[1]))) + 1;
   size_t zCount = static_cast<uint32_t>(ceil(abs(stepCount[2]))) + 1;

   uint32_t* data = new uint32_t[xCount * yCount * zCount];

   //Ensure this always gets deleted if we throw an exception
   Util::finally dataDeleter([data]()
                             {
                                delete[] data;
                             });

   size_t totalCount = xCount * yCount * zCount;

   memset(data, 0, totalCount * sizeof(uint32_t));

   size_t xStride = yCount * zCount;
   size_t yStride = zCount;

   const float* lowerData = lower.Data();

   int outsideCount = 0;

   size_t aCount = m_SnapshotData.m_StateAVnb.size();

   m_ProgressHandler.SetProgressFunc([&outsideCount, aCount]()
                                     {
                                        return "Calculating All-to-All\n" + ion::base::ValueToString(round(outsideCount / static_cast<double>(aCount) * 100.0)) + "% Complete";
                                     });

   bool aborted = false;

#pragma omp parallel for
   for (int aIdx = 0; aIdx < m_SnapshotData.m_StateAVnb.size(); aIdx++)
   {
      const double* stateAdata = m_SnapshotData.m_StateAPos[aIdx].Data();

      const double* stateAvnbX = m_SnapshotData.m_StateAVnb[aIdx][0];
      const double* stateAvnbY = m_SnapshotData.m_StateAVnb[aIdx][1];
      const double* stateAvnbZ = m_SnapshotData.m_StateAVnb[aIdx][2];

#pragma omp flush (aborted)
      if (!aborted && m_CancellationToken == 0)
      {
         try
         {
            for (size_t bIdx = 0; bIdx < m_SnapshotData.m_StateBPos.size(); bIdx++)
            {
               double diff[3];

               const double* stateBdata = m_SnapshotData.m_StateBPos[bIdx].Data();

               diff[0] = (stateBdata[0] - stateAdata[0]);
               diff[1] = (stateBdata[1] - stateAdata[1]);
               diff[2] = (stateBdata[2] - stateAdata[2]);

               double diffF[3];

               diffF[0] = stateAvnbX[0] * diff[0] + stateAvnbX[1] * diff[1] + stateAvnbX[2] * diff[2];
               diffF[1] = stateAvnbY[0] * diff[0] + stateAvnbY[1] * diff[1] + stateAvnbY[2] * diff[2];
               diffF[2] = stateAvnbZ[0] * diff[0] + stateAvnbZ[1] * diff[1] + stateAvnbZ[2] * diff[2];

               diff[0] = (diffF[0] - lowerData[0]) / stepSize;
               diff[1] = (diffF[1] - lowerData[1]) / stepSize;
               diff[2] = (diffF[2] - lowerData[2]) / stepSize;

               if (diff[0] < 0.0f || diff[0] > xCount)
                  throw std::range_error("Point out of boundary");
               if (diff[1] < 0.0f || diff[1] > yCount)
                  throw std::range_error("Point out of boundary");
               if (diff[2] < 0.0f || diff[2] > zCount)
                  throw std::range_error("Point out of boundary");

               size_t finalIndex = xStride * static_cast<size_t>(diff[0]) + yStride * static_cast<size_t>(diff[1]) + static_cast<size_t>(diff[2]);

               if (finalIndex > totalCount)
                  throw std::range_error("Point out of boundary");

#pragma omp atomic
               data[finalIndex]++;
            }
         } catch (std::range_error&)
         {
            aborted = true;
#pragma omp flush (aborted)
         }
      }

#pragma omp atomic
      ++outsideCount;
   }

   if (m_CancellationToken > 0)
      throw cancelled_exception("Cancelled in All-to-All");

   if (aborted || m_CancellationToken > 1)
      throw std::range_error("Point out of boundary");

   for (size_t xIdx = 0; xIdx < xCount; xIdx++)
   {
      for (size_t yIdx = 0; yIdx < yCount; yIdx++)
      {
         for (size_t zIdx = 0; zIdx < zCount; zIdx++)
         {
            uint32_t count = data[xIdx * xStride + yIdx * yStride + zIdx];

            if (count == 0)
               continue;

            m_SnapshotData.m_AllToAllDiffs.push_back(DiffPoint(lower + Point3f(static_cast<float>(xIdx), static_cast<float>(yIdx), static_cast<float>(zIdx)) * static_cast<float>(stepSize), count));
         }
      }
   }
}

std::vector<StateVertex> SnapshotUnitOfWork::GenerateVertices(const std::vector<DiffPoint>& diffData) const
{
   std::vector<StateVertex> vertices;

   float hbr2 = m_HBR * m_HBR;

   size_t i = 0;
   size_t diffCount = diffData.size();

   m_ProgressHandler.SetProgressFunc([&i, diffCount]()
                                     {
                                        return "Generating Vertex Data\n" + ion::base::ValueToString(round(i / static_cast<double>(diffCount) * 100.0)) + "% Complete";
                                     });

   for (i = 0; i < diffData.size(); i++)
   {
      StateVertex vertex;
      vertex.Pos = Vector3f::ToVector(Point3f(diffData[i].Pos));

      if (LengthSquared(vertex.Pos) < hbr2)
         vertex.Color = Vector4ui8(255, 0, 0, 255);
      else
         vertex.Color = Vector4ui8(0, 0, 255, 100);

      vertices.push_back(vertex);
   }

   m_ProgressHandler.SetProgressFunc(nullptr);

   return vertices;
}

SnapshotDataStats SnapshotUnitOfWork::CalcStats(const std::vector<DiffPoint>& diffs) const
{
   SnapshotDataStats stats;

   Point3f minPoint = Point3f::Fill(std::numeric_limits<float>::max());
   Point3f maxPoint = Point3f::Fill(-std::numeric_limits<float>::max());

   float minMiss = std::numeric_limits<float>::max();
   float maxMiss = 0.0f;

   float hbr2 = m_HBR * m_HBR;

   long long hitCount = 0;

   for (size_t i = 0; i < diffs.size(); i++)
   {
      const Point3f& diff = diffs[i].Pos;

      for (int j = 0; j < 3; j++)
      {
         if (diff[j] < minPoint[j])
            minPoint[j] = diff[j];
         if (diff[j] > maxPoint[j])
            maxPoint[j] = diff[j];
      }

      float miss2 = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];

      if (miss2 < minMiss)
      {
         minMiss = miss2;
         stats.MinMiss = diff;
      }

      if (miss2 > maxMiss)
      {
         maxMiss = miss2;
         stats.MaxMiss = diff;
      }

      if (miss2 <= hbr2)
         hitCount += diffs[i].Count;

      stats.Count += diffs[i].Count;
   }

   stats.Bounds = Range3f(Point3f(minPoint), Point3f(maxPoint));
   stats.Pc = static_cast<double>(hitCount) / static_cast<double>(stats.Count);

   return stats;
}

Matrix3d SnapshotData::CalcVNB(const State& origin)
{
   Vector3d r = Normalized(Vector3d(origin.Pos[0], origin.Pos[1], origin.Pos[2]));
   Vector3d v = Normalized(origin.Vel);

   Vector3d n = Normalized(Cross(r, v));
   Vector3d b = Normalized(Cross(v, n));

   return Matrix3d(v[0], v[1], v[2],
                   n[0], n[1], n[2],
                   b[0], b[1], b[2]);
}

FileManager::FileManager():
   m_CurrentEpochIndex(0),
   m_AllToAll(false),
   m_HBR(.120f) {}

FileManager::~FileManager() {}

void FileManager::Cancel()
{
   //Start new thread to cancel since it may have to wait until some processing is done. Must Detach!!!
   std::thread([=]()
               {
                  {
                     //Lock on the cancellation count
                     ion::base::GenericLockGuard<ion::port::Mutex> cancelGuard(&m_CancelMutex);

                     //Increment the cancellation count by 1 to break any processing
                     ++m_CancellationCount;
                  }

                  //Synchronize so only a single thread is active at a time
                  ion::base::GenericLockGuard<ion::port::Mutex> loadGuard(&m_LoadMutex);

                  //must also decrement the counter before exiting
                  --m_CancellationCount;
               }).detach();
}

void FileManager::AddPostProcessStep(const std::string& name, const PostProcess& func)
{
   m_PostProcessSteps[name] = PostProcessInfo(name, func, true);
}

void FileManager::SetHud(const std::shared_ptr<Hud>& hud)
{
   m_Hud = hud;
}

void FileManager::SetFiles(const std::vector<std::string>& files)
{
   //Start new thread to load the changes. Must Detach!!!
   std::thread([=]()
               {
                  {
                     ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_InputChangedMutex);

                     m_Files = files;

                     m_InputChanged = true;
                  }

                  Load();
               }).detach();
}

void FileManager::SetEpochIndex(size_t epochIndex)
{
   //Start new thread to load the changes. Must Detach!!!
   std::thread([=]()
               {
                  {
                     ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_CalcChangedMutex);

                     m_CurrentEpochIndex = epochIndex;
                  }

                  Load();
               }).detach();
}

void FileManager::SetAllToAll(bool allToAll)
{
   //Start new thread to load the changes. Must Detach!!!
   std::thread([=]()
               {
                  {
                     ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_CalcChangedMutex);

                     if (m_AllToAll == allToAll)
                        return;

                     m_AllToAll = allToAll;
                  }

                  Load();
               }).detach();
}

void FileManager::SetHbr(float hbr)
{
   //Start new thread to load the changes. Must Detach!!!
   std::thread([=]()
               {
                  {
                     ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_OutputChangedMutex);

                     if (m_HBR == hbr)
                        return;

                     m_HBR = hbr;
                  }

                  Load();
               }).detach();
}

vector<double> FileManager::GetEpochs() const
{
   vector<double> epochs;

   for (size_t i = 0; i < m_StateData.size(); i++)
   {
      epochs.push_back(m_StateData[i].GetEpoch());
   }

   return epochs;
}

vector<State> FileManager::ParseToStates(const string& filename)
{
   vector<State> states;

   ifstream fs(filename, ifstream::in);

   if (!fs.is_open())
   {
      LOG(ERROR) << "Error opening file: " << filename;
   }

   string line;
   while (getline(fs, line))
   {
      //Do with temp
      vector<string> split = ion::base::SplitString(line, " ,");

      if (split.size() == 8)
      {
         State state;
         state.ScIdx = atoi(split[0].c_str());
         state.Epoch = atof(split[1].c_str());
         state.Pos = Point3d(atof(split[2].c_str()), atof(split[3].c_str()), atof(split[4].c_str()));
         state.Vel = Vector3d(atof(split[5].c_str()), atof(split[6].c_str()), atof(split[7].c_str()));

         states.push_back(state);
      }
   }

   return states;
}

vector<SnapshotData> FileManager::SeparateStateData(const vector<State>& stateLines)
{
   map<double, map<uint16_t, vector<State>>> epochToStateId;

   for (size_t i = 0; i < stateLines.size(); i++)
   {
      if (epochToStateId.find(stateLines[i].Epoch) == epochToStateId.end())
         epochToStateId[stateLines[i].Epoch] = map<uint16_t, vector<State>>();

      auto& epochMap = epochToStateId[stateLines[i].Epoch];

      if (epochMap.find(stateLines[i].ScIdx) == epochMap.end())
         epochMap[stateLines[i].ScIdx] = vector<State>();

      auto& stateMap = epochMap[stateLines[i].ScIdx];

      stateMap.push_back(stateLines[i]);
      stateMap.back().SampleIdx = static_cast<int>(stateMap.size()) - 1;
   }

   vector<SnapshotData> stateData;

   for (auto iter = epochToStateId.begin(); iter != epochToStateId.end(); ++iter)
   {
      if (iter->second.size() == 2)
         stateData.push_back(SnapshotData(iter->first, iter->second.begin()->second, (++iter->second.begin())->second));
   }

   return stateData;
}

void FileManager::Load()
{
   {
      //Lock on the cancellation count
      ion::base::GenericLockGuard<ion::port::Mutex> cancelGuard(&m_CancelMutex);

      //Increment the cancellation count by 1
      ++m_CancellationCount;

      //Release cancellation count lock
   }

   //Synchronize so only a single thread is active at a time
   ion::base::GenericLockGuard<ion::port::Mutex> loadGuard(&m_LoadMutex);

   //Subtract one from the cancellation count
   --m_CancellationCount;

   if (m_CancellationCount > 0)
      return;

   auto progressHandler = m_Hud->GetProgressHudItem()->GetProgressHandler();

   {
      //If files need to be loaded, do that here
      ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_InputChangedMutex);

      try
      {
         if (m_InputChanged)
         {
            vector<State> combinedParsed;

            size_t fileCount = m_Files.size();

            for (size_t i = 0; i < fileCount; i++)
            {
               progressHandler.SetProgressFunc([=]()
                                               {
                                                  return "Reading file " + ion::base::ValueToString(i + 1) + " of " + ion::base::ValueToString(fileCount);
                                               });

               vector<State> parsed = ParseToStates(m_Files[i]);

               if (parsed.size() > 0)
               {
                  combinedParsed.insert(combinedParsed.end(), parsed.begin(), parsed.end());
               }

               if (m_CancellationCount > 0)
                  throw cancelled_exception("Cancelled reading files");
            }

            progressHandler.SetProgressFunc(nullptr);

            m_StateData = SeparateStateData(combinedParsed);
         }

         m_InputChanged = false;
      } catch (cancelled_exception& c)
      {
         LOG(INFO) << c.what();
      }
   }

   if (m_CancellationCount > 0)
      return;

   vector<SnapshotUnitOfWork> workUnits;

   //For each state data, create a unit of work
   for (size_t i = 0; i < m_StateData.size(); i++)
   {
      workUnits.push_back(SnapshotUnitOfWork(m_StateData[i], m_AllToAll, m_HBR, progressHandler, m_CancellationCount));
   }

   //Load the current data for the current index
   size_t index = std::min(std::max(m_CurrentEpochIndex, static_cast<size_t>(0)), m_StateData.size() - 1);

   auto& work = workUnits[index];

   {
      //If files need to be loaded, do that here
      ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_CalcChangedMutex);

      try
      {
         //Load the current indexes data
         work.CalcDiffs();
      } catch (cancelled_exception& c)
      {
         LOG(INFO) << c.what();
      }
   }

   if (m_CancellationCount > 0)
      return;

   //Generate the output for the current index
   {
      //If files need to be loaded, do that here
      ion::base::GenericLockGuard<ion::port::Mutex> lock(&m_OutputChangedMutex);
      try
      {
         //Load the current indexes output data
         work.CalcOutputs();
      } catch (cancelled_exception& c)
      {
         LOG(INFO) << c.what();
      }
   }

   if (m_CancellationCount > 0)
      return;

   //Finally, entering the critical section. No more cancellations can happen here
   {
      //Lock on the cancellation count
      ion::base::GenericLockGuard<ion::port::Mutex> cancelGuard(&m_CancelMutex);

      //Execute all post processing steps
      for (auto iter = m_PostProcessSteps.cbegin(); iter != m_PostProcessSteps.cend(); ++iter)
      {
         if (iter->second.Enabled)
            iter->second.Func(work.GetSnapshotData());
      }

      //Release cancellation count lock
   }
}
}
