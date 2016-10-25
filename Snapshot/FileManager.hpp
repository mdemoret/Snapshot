#pragma once

#include "IonFwd.h"
#include "ion/math/vector.h"
#include <fstream>
#include <map>
#include "ion/math/matrix.h"

class Camera;

namespace Snapshot{
class State
{
public:
   uint16_t ScIdx;
   int SampleIdx = -1;
   double Epoch;
   ion::math::Point3d Pos;
   ion::math::Vector3d Vel;
};

struct StateVertex
{
   ion::math::Vector3f Pos;
   ion::math::Vector3ui Color;
};

class SnapshotData
{
public:
   SnapshotData(double epoch, const std::map<uint16_t, std::vector<State>> & stateData);
   virtual ~SnapshotData();

   double GetEpoch() const;

   std::vector<StateVertex> GetDiffData(uint16_t state1Id, uint16_t state2Id) const;

private:
   static ion::math::Matrix3d CalcVNB(const State & origin);

   double m_Epoch;
   std::map<uint16_t, std::vector<State>> m_StateData;
};

class FileManager
{
public:
   FileManager();
   virtual ~FileManager();

   std::vector<double> GetEpochs() const;

   bool LoadFiles(const std::vector<std::string>& files);

   ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> GetData(size_t epochIndex, uint16_t state1Id, uint16_t state2Id, const ion::base::AllocatorPtr& container_allocator);

protected:
   

private:
   static std::vector<State> ParseToStates(const std::string & filename);
   static std::vector<SnapshotData> SeparateStateData(const std::vector<State> & stateLines);
  
   std::vector<std::string> m_Files;
   std::vector<SnapshotData> m_StateData;
};
}
