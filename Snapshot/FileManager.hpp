#pragma once

#include "IonFwd.h"
#include "ion/math/vector.h"
#include <fstream>
#include <map>
#include "ion/math/matrix.h"
#include "ion/base/notifier.h"
#include "ion/math/range.h"

class Hud;
class Camera;

class ProgressHandler;

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

   StateVertex(const ion::math::Vector3f & pos, const ion::math::Vector4ui8 & color) :
      Pos(pos),
      Color(color)
   {}

   ion::math::Vector3f Pos;
   ion::math::Vector4ui8 Color;
};

struct DiffPoint
{
public:
   DiffPoint(): Count(0) {}

   DiffPoint(const ion::math::Point3f & pos, uint32_t count) :
      Pos(pos),
      Count(count)
   {}

   ion::math::Point3f Pos;
   uint32_t Count;
};

struct SnapshotDataStats
{
   SnapshotDataStats();

   size_t Count;
   ion::math::Range3f Bounds;
   ion::math::Vector3f CenterOfMass;
   ion::math::Point3f MinMiss;
   ion::math::Point3f MaxMiss;
   double Pc;
};

class SnapshotData
{
public:
   SnapshotData(double epoch, const std::vector<State> & stateA, const std::vector<State> & stateB);
   virtual ~SnapshotData();

   double GetEpoch() const;
   //const ion::math::Range3f & GetBounds() const;
   const SnapshotDataStats & GetStats() const;
   const std::vector<StateVertex> & GetOutputData() const;

   //std::vector<StateVertex> GetDiffData() const;

   static ion::math::Matrix3d CalcVNB(const State & origin);

private:
   friend class SnapshotUnitOfWork;

   //static std::vector<StateVertex> GenerateVertices(const std::vector<DiffPoint> & diffData, float hbr, ProgressHandler & progress);

   //std::vector<StateVertex> Calculate1to1(float hbr) const;
   //std::vector<StateVertex> CalculateAtoA(float hbr) const;

   //const std::vector<DiffPoint> & GetOneToOneData(ProgressHandler * progress = nullptr) const;
   //const std::vector<DiffPoint> & GetAllToAllData(ProgressHandler * progress = nullptr) const;

   const double m_Epoch;
   const std::vector<ion::math::Matrix3d> m_StateAVnb;
   const std::vector<ion::math::Point3d> m_StateAPos;
   const std::vector<ion::math::Point3d> m_StateBPos;

   //The following are set on initialization
   //const ion::math::Range3d m_StateABounds;
   //const ion::math::Range3d m_StateBBounds;

   //Calculated diff data
   mutable std::vector<DiffPoint> m_OneToOneDiffs;
   mutable std::vector<DiffPoint> m_AllToAllDiffs;

   //Output Data
   std::vector<StateVertex> m_OutputData;
   SnapshotDataStats m_Stats;
};

class cancelled_exception : public std::runtime_error
{
public:
   explicit cancelled_exception(const string& _Message)
      : runtime_error(_Message) {}

   explicit cancelled_exception(const char* const _Message)
      : runtime_error(_Message) {}
};

class SnapshotUnitOfWork
{
public:
   SnapshotUnitOfWork(SnapshotData & snapshotData, bool allToAll, float hbr, ProgressHandler & progressHandler, std::atomic<uint32_t> & cancellationToken);

   void CalcDiffs() const;
   void CalcOutputs() const;

   SnapshotData & GetSnapshotData() const;

   const std::vector<DiffPoint> & GetOneToOneData() const;
   const std::vector<DiffPoint> & GetAllToAllData() const;
   
private:
   void TryBinAllToAllData(const ion::math::Range3f & bounds, float multiplier, uint32_t binCount) const;

   std::vector<StateVertex> GenerateVertices(const std::vector<DiffPoint> & diffData) const;
   SnapshotDataStats CalcStats(const std::vector<DiffPoint> & diffs) const;

   SnapshotData & m_SnapshotData;
   bool m_AllToAll;
   float m_HBR;
   ProgressHandler & m_ProgressHandler;
   std::atomic<uint32_t> & m_CancellationToken;
};

class FileManager : public ion::base::Notifier
{
public:
   // A function that is called when the value changes.
   typedef std::function<void(const SnapshotData & snapshot)> PostProcess;
   struct PostProcessInfo
   {
      PostProcessInfo() : 
         Name(""),
         Func(nullptr),
         Enabled(false)
      {}

      PostProcessInfo(const std::string & name, const PostProcess& func, bool enabled) : 
         Name(name),
         Func(func),
         Enabled(enabled)
      {}
      std::string Name;
      PostProcess Func;
      bool Enabled;
   };

   FileManager();
   virtual ~FileManager();

   void Cancel();

   void AddPostProcessStep(const std::string & name, const PostProcess& func);

   void SetHud(const std::shared_ptr<Hud> & hud);

   void SetFiles(const std::vector<std::string>& files);
   void SetEpochIndex(size_t epochIndex);
   void SetAllToAll(bool allToAll);
   void SetHbr(float hbr);

   std::vector<double> GetEpochs() const;

protected:
   

private:
   static std::vector<State> ParseToStates(const std::string & filename);
   static std::vector<SnapshotData> SeparateStateData(const std::vector<State> & stateLines);

   //static std::vector<SnapshotData> LoadInput(const std::vector<std::string> & files, ProgressHandler & progress);
   //static void CalcDiffs(SnapshotData & data, bool allToAll, float hbr, ProgressHandler & progress);
   //static void CalcOutputs(SnapshotData & data, bool allToAll, float hbr, ProgressHandler & progress);

   void Load();
  
   std::shared_ptr<Hud> m_Hud;

   //Sychronization objects
   ion::port::Mutex m_CancelMutex;
   std::atomic<uint32_t> m_CancellationCount;
   ion::port::Semaphore m_LoadSemaphore;
   ion::port::Mutex m_LoadMutex;

   //Inputs
   ion::port::Mutex m_InputChangedMutex;
   std::atomic<bool> m_InputChanged;
   std::vector<std::string> m_Files;

   //Calculations
   ion::port::Mutex m_CalcChangedMutex;
   size_t m_CurrentEpochIndex;
   bool m_AllToAll;

   //Outputs
   ion::port::Mutex m_OutputChangedMutex;
   float m_HBR;

   std::vector<SnapshotData> m_StateData;

   std::map<std::string, PostProcessInfo> m_PostProcessSteps;
};
}
