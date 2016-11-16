#pragma once

#include "SceneBase.hpp"
#include "ion/base/setting.h"

class Hud;

namespace Snapshot{
class FileManager;
class KeyboardHandler;

class Scene :
   public SceneBase
{
public:
   explicit Scene(KeyboardHandler & keyboard);
   virtual ~Scene();

   virtual bool Update(double elapsedTimeInSec, double secSinceLastFrame) override;

protected:
   ion::gfx::NodePtr BuildSimpleAxes(const ion::gfxutils::ShaderManagerPtr& shaderManager);

   void RenderFrame() override;

private:
   ion::gfx::NodePtr BuildWorldSceneGraph();
   ion::gfx::NodePtr BuildHudSceneGraph();

   ion::gfx::NodePtr m_WorldRoot;
   ion::gfx::NodePtr m_HudRoot;
   std::shared_ptr<Hud> m_Hud;

   ion::gfx::NodePtr BuildPointsNode();
   //ion::gfx::BufferObjectPtr m_PointsBuffer;
   //ion::gfx::NodePtr m_PointsNode;

   ion::gfx::NodePtr BuildMissDistNode();
   //ion::gfx::BufferObjectPtr m_MissDistBuffer;
   //ion::gfx::NodePtr m_MissDistNode;

   std::shared_ptr<FileManager> m_FileManager;
   
   ion::base::Setting<std::vector<std::string>> m_InputFiles;
   ion::base::Setting<uint32_t> m_EpochIndex;

   ion::base::Setting<float> m_HardBodyRadius;
   ion::base::Setting<bool> m_AllToAll_Use;
   ion::base::Setting<uint32_t> m_AllToAll_BinCount;
   ion::base::Setting<float> m_AllToAll_InitialBinMultiplier;
   ion::base::Setting<uint32_t> m_AllToAll_MaxBinTries;
   
   ion::base::Setting<bool> m_LookAtCOM;
   ion::base::Setting<bool> m_ShowCOM;
   ion::base::Setting<bool> m_AutoScale;
   ion::base::Setting<float> m_PointSize;
};
}
