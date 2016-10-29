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
SnapshotData::SnapshotData(double epoch, const map<uint16_t, vector<State>>& stateData) :
   m_Epoch(epoch),
   m_StateData(stateData) {}

SnapshotData::~SnapshotData() {}

double SnapshotData::GetEpoch() const
{
   return m_Epoch;
}

const Range3f& SnapshotData::GetBounds() const
{
   return m_DifferenceBounds;
}

vector<StateVertex> SnapshotData::GetDiffData(uint16_t state1Id, uint16_t state2Id) const
{
   auto& left = m_StateData.find(state1Id)->second;
   auto& right = m_StateData.find(state2Id)->second;

   bool useAllToAll = dynamic_cast<ion::base::Setting<bool>*>(ion::base::SettingManager::GetSetting(SETTINGS_SCENE_ALLTOALL))->GetValue();
   float hbr = dynamic_cast<ion::base::Setting<float>*>(ion::base::SettingManager::GetSetting(SETTINGS_HBR))->GetValue() / 1000.0f;

   if (useAllToAll)
      return CalculateAtoA(left, right, hbr);
   else
      return Calculate1to1(left, right, hbr);
}

vector<StateVertex> SnapshotData::Calculate1to1(const vector<State>& left, const vector<State>& right, float hbr) const
{
   vector<StateVertex> vertices;

   for (size_t i = 0; i < right.size(); i++)
   {
      Matrix3d rot = CalcVNB(right[i]);

      Vector3d leftVNB = rot * (left[i].Pos - right[i].Pos);

      StateVertex vertex;
      vertex.Pos = Vector3f(leftVNB);

      if (Length(vertex.Pos) < hbr)
         vertex.Color = Vector3ui8(255, 0, 0);
      else
         vertex.Color = Vector3ui8::Fill(255);

      vertices.push_back(vertex);
   }

   return vertices;
}

vector<StateVertex> SnapshotData::CalculateAtoA(const vector<State>& left, const vector<State>& right, float hbr) const
{
   map<float, map<float, map<float, uint32_t>>> bins;

   const int scale = 17;

   long long maxBin = 0;

   for (size_t rIdx = 0; rIdx < right.size(); rIdx++)
   {
      Matrix3d vnbRot = CalcVNB(right[rIdx]);

      for (size_t lIdx = 0; lIdx < left.size(); lIdx++)
      {
         Vector3d leftVNB = vnbRot * (left[lIdx].Pos - right[rIdx].Pos);

         union
      {
         int ix;
         float fx;
      } x, y, z;

         x.fx = static_cast<float>(leftVNB[0]);
         x.ix &= ~((1 << scale) - 1);

         y.fx = static_cast<float>(leftVNB[1]);
         y.ix &= ~((1 << scale) - 1);

         z.fx = static_cast<float>(leftVNB[2]);
         z.ix &= ~((1 << scale) - 1);

         maxBin = max(static_cast<long long>(bins[x.fx][y.fx][z.fx]++), maxBin);
      }
   }

   m_DifferenceBounds = Range3f(Point3f(bins.begin()->first, std::numeric_limits<float>::max(), std::numeric_limits<float>::max()), Point3f(bins.rbegin()->first, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()));

   vector<StateVertex> vertices;

   for (auto xIter = bins.begin(); xIter != bins.end(); ++xIter)
   {
      if (xIter->second.begin()->first < m_DifferenceBounds.GetMinPoint()[1])
         m_DifferenceBounds.SetMinComponent(1, xIter->second.begin()->first);

      if (xIter->second.rbegin()->first > m_DifferenceBounds.GetMaxPoint()[1])
         m_DifferenceBounds.SetMaxComponent(1, xIter->second.rbegin()->first);

      for (auto yIter = xIter->second.begin(); yIter != xIter->second.end(); ++yIter)
      {
         if (yIter->second.begin()->first < m_DifferenceBounds.GetMinPoint()[2])
            m_DifferenceBounds.SetMinComponent(2, yIter->second.begin()->first);

         if (yIter->second.rbegin()->first > m_DifferenceBounds.GetMaxPoint()[2])
            m_DifferenceBounds.SetMaxComponent(2, yIter->second.rbegin()->first);

         for (auto zIter = yIter->second.begin(); zIter != yIter->second.end(); ++zIter)
         {
            StateVertex vertex;
            vertex.Pos = Vector3f(xIter->first, yIter->first, zIter->first);

            if (Length(vertex.Pos) < hbr)
               vertex.Color = Vector3ui8(255, 0, 0);
            else
               vertex.Color = Vector3ui8::Fill(255);

            //vertex.Color = Vector3ui8::Fill(static_cast<uint8_t>(static_cast<double>(zIter->second * 255) / static_cast<double>(maxBin)));

            vertices.push_back(vertex);
         }
      }
   }

   return vertices;
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
      vector<State> parsed = ParseToStates(files[0]);

      if (parsed.size() > 0)
      {
         m_Files.push_back(files[0]);

         combinedParsed.insert(combinedParsed.end(), parsed.begin(), parsed.end());
      }
   }

   m_StateData = SeparateStateData(combinedParsed);

   return m_Files.size() > 0;
}

ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> FileManager::GetData(size_t epochIndex, uint16_t state1Id, uint16_t state2Id, ion::math::Range3f & bounds)
{
   auto& selected = m_StateData[epochIndex];

   ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> vertexData(new ion::base::VectorDataContainer<StateVertex>(true));

   auto diffVector = selected.GetDiffData(state1Id, state2Id);

   vertexData->GetMutableVector()->insert(vertexData->GetMutableVector()->end(), diffVector.begin(), diffVector.end());

   Notify();

   bounds = selected.GetBounds();

   return vertexData;
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
      stateData.push_back(SnapshotData(iter->first, iter->second));
   }

   return stateData;
}
}
