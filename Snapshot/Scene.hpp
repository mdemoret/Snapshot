#pragma once

#include "SceneBase.hpp"
#include "ion/base/setting.h"
#include "FileManager.hpp"

class Hud;

namespace Snapshot{

class Scene :
   public SceneBase
{
public:
   Scene();
   virtual ~Scene();

   virtual void Update() override;

protected:
   ion::gfx::NodePtr BuildSimpleAxes(const ion::gfxutils::ShaderManagerPtr& shaderManager);

   void LoadInputFiles(ion::base::SettingBase* setting);
   void ReloadPointData();

   void RenderFrame() override;

private:
   ion::gfx::NodePtr BuildWorldSceneGraph();
   ion::gfx::NodePtr BuildHudSceneGraph();

   ion::gfx::NodePtr m_WorldRoot;
   ion::gfx::NodePtr m_HudRoot;
   std::shared_ptr<Hud> m_Hud;

   ion::gfx::BufferObjectPtr m_PointsBuffer;
   ion::gfx::NodePtr m_PointsNode;

   FileManager m_FileManager;
   ion::base::Setting<std::vector<std::string>> m_InputFiles;
   ion::base::Setting<float> m_HardBodyRadius;
   ion::base::Setting<size_t> m_EpochIndex;
   ion::base::Setting<bool> m_LookAtCOM;
   ion::base::Setting<bool> m_AutoScale;
   ion::base::Setting<bool> m_UseAllToAll;
};
}
