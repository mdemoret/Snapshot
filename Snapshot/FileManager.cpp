#include "FileManager.hpp"
#include <ion/base/stringutils.h>
#include <ion/base/vectordatacontainer.h>
#include <ion/math/matrixutils.h>
#include <ion/math/vectorutils.h>
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

SnapshotData::SnapshotData(double epoch, const std::vector<State>& stateA, const std::vector<State>& stateB):
   m_Epoch(epoch),
   m_StateAVnb(CalcStatesVnb(stateA)),
   m_StateAPos(CalcStatesPos(stateA)),
   m_StateBPos(CalcStatesPos(stateB)),
   m_StateABounds(CalcStatesRange(stateA)),
   m_StateBBounds(CalcStatesRange(stateB))
{


}
SnapshotData::~SnapshotData() {}

double SnapshotData::GetEpoch() const
{
   return m_Epoch;
}

//const Range3f& SnapshotData::GetBounds() const
//{
//   return m_DifferenceBounds;
//}

vector<StateVertex> SnapshotData::GetDiffData() const
{
   bool useAllToAll = dynamic_cast<ion::base::Setting<bool>*>(ion::base::SettingManager::GetSetting(SETTINGS_SCENE_ALLTOALL))->GetValue();
   float hbr = dynamic_cast<ion::base::Setting<float>*>(ion::base::SettingManager::GetSetting(SETTINGS_HBR))->GetValue() / 1000.0f;

   if (useAllToAll)
      return CalculateAtoA(hbr);
   else
      return Calculate1to1(hbr);
}

vector<StateVertex> SnapshotData::Calculate1to1(float hbr) const
{
   //Get the diff data. This is cached if possible
   LoadOneToOneData();

   //First thing, calculate the maximum size
   const Point3f & upper = m_OneToOneBounds.GetMaxPoint();
   const Point3f & lower = m_OneToOneBounds.GetMinPoint();

   float stepSize = 0.01f;

   //Now determine the number of steps in each direction
   Vector3f stepCount = (upper - lower) / stepSize;

   size_t xCount = (uint32_t)ceil(abs(stepCount[0])) + 1;
   size_t yCount = (uint32_t)ceil(abs(stepCount[1])) + 1;
   size_t zCount = (uint32_t)ceil(abs(stepCount[2])) + 1;

   uint32_t * data = new uint32_t[xCount * yCount * zCount];

   size_t totalCount = xCount * yCount * zCount;

   memset(data, 0, totalCount * sizeof(uint32_t));

   vector<StateVertex> vertices;

   size_t xStride = yCount * zCount;
   size_t yStride = zCount;

   float hbr2 = hbr * hbr;

   for (size_t i = 0; i < m_OneToOneDiffs.size(); i++)
   {
      Vector3f indexFloating = (Point3f(m_OneToOneDiffs[i]) - lower) / stepSize;

      DCHECK_LT(indexFloating[0], xCount);
      DCHECK_LT(indexFloating[1], yCount);
      DCHECK_LT(indexFloating[2], zCount);

      size_t finalIndex = xStride * (size_t)round(indexFloating[0]) + yStride * (size_t)round(indexFloating[1]) + (size_t)round(indexFloating[2]);

      DCHECK_LT(finalIndex, totalCount);

      data[finalIndex]++;
   }

   Vector3f lowerVector = Vector3f::ToVector(lower);

   for (size_t xIdx = 0; xIdx < xCount; xIdx++)
   {
      for (size_t yIdx = 0; yIdx < yCount; yIdx++)
      {
         for (size_t zIdx = 0; zIdx < zCount; zIdx++)
         {
            if (data[xIdx * xStride + yIdx * yStride + zIdx] == 0)
               continue;

            StateVertex vertex;
            vertex.Pos = lowerVector + (Vector3f(static_cast<float>(xIdx), static_cast<float>(yIdx), static_cast<float>(zIdx)) * static_cast<float>(stepSize));

            if (LengthSquared(vertex.Pos) < hbr2)
               vertex.Color = Vector4ui8(255, 0, 0, 255);
            else
               vertex.Color = Vector4ui8(0, 0, 255, 100);

            vertices.push_back(vertex);
         }
      }
   }

   delete[] data;

   return vertices;
}

vector<StateVertex> SnapshotData::CalculateAtoA(float hbr) const
{
   //Get the diff data. This is cached if possible
   LoadOneToOneData();

   //Use the one to one bounds to make a guess on the max size by doing 2X the one to one
   const Point3f & upper = m_OneToOneBounds.GetCenter() + m_OneToOneBounds.GetSize();
   const Point3f & lower = m_OneToOneBounds.GetCenter() - m_OneToOneBounds.GetSize();

   float stepSize = 0.0005f;

   //Now determine the number of steps in each direction
   Vector3f stepCount = (upper - lower) / stepSize;

   size_t xCount = (uint32_t)ceil(abs(stepCount[0])) + 1;
   size_t yCount = (uint32_t)ceil(abs(stepCount[1])) + 1;
   size_t zCount = (uint32_t)ceil(abs(stepCount[2])) + 1;

   uint32_t * data = new uint32_t[xCount * yCount * zCount];

   size_t totalCount = xCount * yCount * zCount;

   memset(data, 0, totalCount * sizeof(uint32_t));

   vector<StateVertex> vertices;

   size_t xStride = yCount * zCount;
   size_t yStride = zCount;

   float hbr2 = hbr * hbr;

   const float * lowerData = lower.Data();

#pragma omp parallel for
   for (int aIdx = 0; aIdx < m_StateAVnb.size(); aIdx++)
   {
      const double * stateAdata = m_StateAPos[aIdx].Data();

      const double * stateAvnbX = m_StateAVnb[aIdx][0];
      const double * stateAvnbY = m_StateAVnb[aIdx][1];
      const double * stateAvnbZ = m_StateAVnb[aIdx][2];

      for (size_t bIdx = 0; bIdx < m_StateBPos.size(); bIdx++)
      {
         Point3d diff2 = Point3d::ToPoint(m_StateAVnb[aIdx] * (m_StateBPos[bIdx] - m_StateAPos[aIdx]));

         Vector3f indexFloating = (Point3f(diff2) - lower) / stepSize;

         //DCHECK_LT(indexFloating[0], xCount);
         //DCHECK_LT(indexFloating[1], yCount);
         //DCHECK_LT(indexFloating[2], zCount);

         //size_t finalIndex = xStride * (size_t)round(indexFloating[0]) + yStride * (size_t)round(indexFloating[1]) + (size_t)round(indexFloating[2]);

         //DCHECK_LT(finalIndex, totalCount);

         //data[finalIndex]++;

         double diff[3];
         
         const double * stateBdata = m_StateBPos[bIdx].Data();

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

         DCHECK_LT(diff[0], xCount);
         DCHECK_LT(diff[1], yCount);
         DCHECK_LT(diff[2], zCount);

         size_t finalIndex = xStride * (size_t)diff[0] + yStride * (size_t)diff[1] + (size_t)diff[2];

         DCHECK_LT(finalIndex, totalCount);

#pragma omp atomic
         data[finalIndex]++;
      }
   }

   Vector3f lowerVector = Vector3f::ToVector(lower);

   for (size_t xIdx = 0; xIdx < xCount; xIdx++)
   {
      for (size_t yIdx = 0; yIdx < yCount; yIdx++)
      {
         for (size_t zIdx = 0; zIdx < zCount; zIdx++)
         {
            if (data[xIdx * xStride + yIdx * yStride + zIdx] == 0)
               continue;

            StateVertex vertex;
            vertex.Pos = lowerVector + (Vector3f(static_cast<float>(xIdx), static_cast<float>(yIdx), static_cast<float>(zIdx)) * static_cast<float>(stepSize));

            if (LengthSquared(vertex.Pos) < hbr2)
               vertex.Color = Vector4ui8(255, 0, 0, 255);
            else
               vertex.Color = Vector4ui8(0, 0, 255, 100);

            vertices.push_back(vertex);
         }
      }
   }

   delete[] data;

   return vertices;
}

void SnapshotData::LoadOneToOneData() const 
{
   if (m_OneToOneDiffs.size() > 0)
      return;

   Point3d minPoint = Point3d::Fill(std::numeric_limits<float>::max());
   Point3d maxPoint = Point3d::Fill(-std::numeric_limits<float>::max());

   for (size_t i = 0; i < m_StateAVnb.size(); i++)
   {
      Point3d diff = Point3d::ToPoint(m_StateAVnb[i] * (m_StateBPos[i] - m_StateAPos[i]));

      for (int j = 0; j < 3; j++)
      {
         if (diff[j] < minPoint[j])
            minPoint[j] = diff[j];
         if (diff[j] > maxPoint[j])
            maxPoint[j] = diff[j];
      }

      m_OneToOneDiffs.push_back(diff);
   }

   m_OneToOneBounds = Range3f(Point3f(minPoint), Point3f(maxPoint));
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

FileManager::FileManager() {}

FileManager::~FileManager() {}

vector<double> FileManager::GetEpochs() const
{
   vector<double> epochs;

   for (size_t i = 0; i < m_StateData.size(); i++)
   {
      epochs.push_back(m_StateData[i].GetEpoch());
   }

   return epochs;
}

bool FileManager::LoadFiles(const vector<string>& files)
{
   m_Files.clear();

   vector<State> combinedParsed;

   for (size_t i = 0; i < files.size(); i++)
   {
      vector<State> parsed = ParseToStates(files[i]);

      if (parsed.size() > 0)
      {
         m_Files.push_back(files[i]);

         combinedParsed.insert(combinedParsed.end(), parsed.begin(), parsed.end());
      }
   }

   m_StateData = SeparateStateData(combinedParsed);

   return m_Files.size() > 0;
}

const SnapshotData& FileManager::GetData(size_t epochIndex) const
{
   return m_StateData[epochIndex];
}
//ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> FileManager::GetData(size_t epochIndex, uint16_t state1Id, uint16_t state2Id, ion::math::Range3f & bounds)
//{
//   auto& selected = m_StateData[epochIndex];
//
//   ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> vertexData(new ion::base::VectorDataContainer<StateVertex>(true));
//
//   auto diffVector = selected.GetDiffData();
//
//   vertexData->GetMutableVector()->insert(vertexData->GetMutableVector()->end(), diffVector.begin(), diffVector.end());
//
//   Notify();
//
//   bounds = selected.GetBounds();
//
//   return vertexData;
//}

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
}
