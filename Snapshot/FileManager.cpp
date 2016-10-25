#include "FileManager.hpp"
#include <ion/base/stringutils.h>
#include <ion/base/vectordatacontainer.h>
#include <ion/math/matrixutils.h>
#include <ion/math/vectorutils.h>

using namespace ion::math;

namespace Snapshot
{
SnapshotData::SnapshotData(double epoch, const std::map<uint16_t, std::vector<State>>& stateData) :
   m_Epoch(epoch),
   m_StateData(stateData)
{}
SnapshotData::~SnapshotData() {}

double SnapshotData::GetEpoch() const {
   return m_Epoch;
}

std::vector<StateVertex> SnapshotData::GetDiffData(uint16_t state1Id, uint16_t state2Id) const 
{
   auto & left = m_StateData.find(state1Id)->second;
   auto & right = m_StateData.find(state2Id)->second;

   std::vector<StateVertex> vertices;

   for (size_t i = 0; i < right.size(); i++)
   {
      Matrix3d rot = CalcVNB(right[i]);

      Vector3d leftVNB = rot * (left[i].Pos - right[i].Pos);

      StateVertex vertex;
      vertex.Pos = Vector3f(leftVNB);
      vertex.Color = Vector3ui::Fill(256);

      vertices.push_back(vertex);
   }

   return vertices;
}

ion::math::Matrix3d SnapshotData::CalcVNB(const State& origin)
{
   Vector3d r = ion::math::Normalized(Vector3d(origin.Pos[0], origin.Pos[1], origin.Pos[2]));
   Vector3d v = ion::math::Normalized(origin.Vel);

   Vector3d n = ion::math::Normalized(Cross(r, v));
   Vector3d b = ion::math::Normalized(Cross(v, n));

   return Matrix3d(v[0], v[1], v[2],
                   n[0], n[1], n[2],
                   b[0], b[1], b[2]);
}

FileManager::FileManager() 
{}

FileManager::~FileManager() 
{}

std::vector<double> FileManager::GetEpochs() const 
{
   std::vector<double> epochs;

   for (size_t i = 0; i < m_StateData.size(); i++)
   {
      epochs.push_back(m_StateData[i].GetEpoch());
   }

   return epochs;
}

bool FileManager::LoadFiles(const std::vector<std::string>& files)
{
   m_Files.clear();

   std::vector<State> combinedParsed;

   for (size_t i = 0; i < files.size(); i++)
   {
      std::vector<State> parsed = ParseToStates(files[0]);

      if (parsed.size() > 0)
      {
         m_Files.push_back(files[0]);

         combinedParsed.insert(combinedParsed.end(), parsed.begin(), parsed.end());
      }
   }

   m_StateData = SeparateStateData(combinedParsed);

   return m_Files.size() > 0;
}

ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> FileManager::GetData(size_t epochIndex, uint16_t state1Id, uint16_t state2Id, const ion::base::AllocatorPtr& container_allocator)
{
   auto & selected = m_StateData[epochIndex];

   ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> vertexData(new ion::base::VectorDataContainer<StateVertex>(true));

   auto diffVector = selected.GetDiffData(state1Id, state2Id);

   vertexData->GetMutableVector()->insert(vertexData->GetMutableVector()->end(), diffVector.begin(), diffVector.end());

   return vertexData;
}

std::vector<State> FileManager::ParseToStates(const std::string& filename) 
{
   std::vector<State> states;

   std::ifstream fs(filename, std::ifstream::in);

   if (fs.is_open())
   {
      LOG(ERROR) << "Error opening file: " << filename;
   }

   std::string line;
   while (std::getline(fs, line))
   {
      //Do with temp
      std::vector<string> split = ion::base::SplitString(line, " ,");

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

std::vector<SnapshotData> FileManager::SeparateStateData(const std::vector<State>& stateLines)
{
   std::map<double, std::map<uint16_t, std::vector<State>>> epochToStateId;

   for (size_t i = 0; i < stateLines.size(); i++)
   {
      if (epochToStateId.find(stateLines[i].Epoch) == epochToStateId.end())
         epochToStateId[stateLines[i].Epoch] = std::map<uint16_t, std::vector<State>>();

      auto & epochMap = epochToStateId[stateLines[i].Epoch];

      if (epochMap.find(stateLines[i].ScIdx) == epochMap.end())
         epochMap[stateLines[i].ScIdx] = std::vector<State>();

      auto & stateMap = epochMap[stateLines[i].ScIdx];

      stateMap.push_back(stateLines[i]);
      stateMap.back().SampleIdx = static_cast<int>(stateMap.size()) - 1;
   }

   std::vector<SnapshotData> stateData;

   for (auto iter = epochToStateId.begin(); iter != epochToStateId.end(); ++iter)
   {
      stateData.push_back(SnapshotData(iter->first, iter->second));
   }

   return stateData;
}
}
