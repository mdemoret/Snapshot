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
   void LoadInputFiles(ion::base::SettingBase* setting);

   void RenderFrame() override;

private:
   void BuildWorldSceneGraph();
   void BuildHudSceneGraph();

   ion::gfx::NodePtr m_WorldRoot;
   ion::gfx::NodePtr m_HudRoot;
   std::shared_ptr<Hud> m_Hud;

   FileManager m_FileManager;
   ion::base::Setting<std::vector<std::string>> m_InputFiles;
};
}
