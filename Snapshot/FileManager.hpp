#pragma once

#include "IonFwd.h"
#include "ion/math/vector.h"
#include <fstream>
#include <map>
#include "ion/math/matrix.h"
#include "ion/base/notifier.h"
#include "ion/math/range.h"

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
public:
   StateVertex()
   {}

   StateVertex(const ion::math::Vector3f & pos, const ion::math::Vector3ui8 & color) :
      Pos(pos),
      Color(color)
   {}

   ion::math::Vector3f Pos;
   ion::math::Vector3ui8 Color;
};

class SnapshotData
{
public:
   SnapshotData(double epoch, const std::map<uint16_t, std::vector<State>> & stateData);
   virtual ~SnapshotData();

   double GetEpoch() const;
   const ion::math::Range3f & GetBounds() const;

   std::vector<StateVertex> GetDiffData(uint16_t state1Id, uint16_t state2Id) const;

private:
   std::vector<StateVertex> Calculate1to1(const std::vector<State> & left, const std::vector<State> & right, float hbr) const;
   std::vector<StateVertex> CalculateAtoA(const std::vector<State> & left, const std::vector<State> & right, float hbr) const;

   static ion::math::Matrix3d CalcVNB(const State & origin);

   double m_Epoch;
   std::map<uint16_t, std::vector<State>> m_StateData;
   mutable ion::math::Range3f m_DifferenceBounds;
};

class FileManager : public ion::base::Notifier
{
public:
   FileManager();
   virtual ~FileManager();

   std::vector<double> GetEpochs() const;

   bool LoadFiles(const std::vector<std::string>& files);

   ion::base::SharedPtr<ion::base::VectorDataContainer<StateVertex>> GetData(size_t epochIndex, uint16_t state1Id, uint16_t state2Id, ion::math::Range3f & bounds);

protected:
   

private:
   static std::vector<State> ParseToStates(const std::string & filename);
   static std::vector<SnapshotData> SeparateStateData(const std::vector<State> & stateLines);
  
   std::vector<std::string> m_Files;
   std::vector<SnapshotData> m_StateData;
};
}
