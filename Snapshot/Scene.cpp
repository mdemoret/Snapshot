#include "Scene.hpp"
#include "ion/gfx/node.h"
#include "Camera.hpp"
#include <ion/gfx/renderer.h>
#include <ion/base/setting.h>
#include "Macros.h"
#include <ion/gfxutils/shapeutils.h>
#include <ion/gfx/shaderinputregistry.h>
#include "Hud.hpp"
#include <ion/text/fontmanager.h>
#include <ion/gfxutils/buffertoattributebinder.h>
#include <ion/base/vectordatacontainer.h>
#include <ion/base/zipassetmanagermacros.h>
#include "ion/gfxutils/shadermanager.h"
#include "ion/base/zipassetmanager.h"

// Resources for the HUD.
ION_REGISTER_ASSETS(SnapshotAssets);

using namespace ion::gfx;
using namespace ion::math;
using namespace ion::base;

namespace Snapshot{

Scene::Scene():
   SceneBase(),
   m_InputFiles(SETTINGS_INPUT_FILES, std::vector<std::string>(), "Sets the relative files to load into the snapshot tool")
{
   SnapshotAssets::RegisterAssetsOnce();

   auto names = ion::base::ZipAssetManager::GetRegisteredFileNames();

   auto data = ion::base::ZipAssetManager::GetFileData("point.vert");

   using std::bind;
   using std::placeholders::_1;

   //Add listenr for setting
   m_InputFiles.RegisterListener("LoadInputFiles", bind(&Scene::LoadInputFiles, this, _1));

   //Build Scene Graph
   //Initialize the 3d and Hud nodes
   m_WorldRoot = NodePtr(new Node());
   m_WorldRoot->SetLabel("World Root");
   GetRoot()->AddChild(m_WorldRoot);

   m_Hud = std::make_shared<Hud>(ion::text::FontManagerPtr(new ion::text::FontManager), GetShaderManager(), GetCamera()->GetViewportUniforms());

   m_Hud->AddHudItem(std::make_shared<HudItem>("BFMC Snapshot"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("@ 12/09/2016 08:58:27.379"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("Pc = 5.4e-04"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("RTm1A (Origin) RTm4A"));

   m_Hud->GetRootNode()->SetLabel("HUD");

   m_HudRoot = m_Hud->GetRootNode();
   GetRoot()->AddChild(m_HudRoot);

   //Add the uniforms for 3d rotation
   m_WorldRoot->AddUniformBlock(GetCamera()->Get3dUniforms());

   BuildWorldSceneGraph();
   BuildHudSceneGraph();
}

Scene::~Scene() {}

void Scene::Update()
{
   SceneBase::Update();

   //Occurs once per loop. May or may not draw the frame after
   m_Hud->Update();
}

void Scene::LoadInputFiles(ion::base::SettingBase* setting)
{
   //First attempt to read the files
   if (!m_FileManager.LoadFiles(static_cast<ion::base::Setting<std::vector<std::string>>*>(setting)->GetValue()))
   {
      //Error
      LOG(ERROR) << "Error reading files";
      return;
   }

   //Now create the particles
   AttributeArrayPtr attribute_array(new AttributeArray);
   ShaderInputRegistryPtr registry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   registry->IncludeGlobalRegistry();

   registry->Add(ion::gfx::ShaderInputRegistry::UniformSpec("uPointSize", ion::gfx::kFloatUniform, "The size in pixels of the point"));

   BufferObjectPtr buffer_object(new BufferObject);
   auto container = m_FileManager.GetData(0, 1, 0, buffer_object->GetAllocator());

   buffer_object->SetData(container, sizeof(StateVertex), container->GetVector().size(), BufferObject::kStaticDraw);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, "aVertex")
      .Bind(v.Color, "aColor")
      .Apply(registry, attribute_array, buffer_object);

   ShaderProgramPtr shader = GetShaderManager()->CreateShaderProgram("Point",
                                                                     registry,
                                                                     ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("point.vert", false)),
                                                                     ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("point.frag", false)));


   ShapePtr shape(new Shape);
   shape->SetPrimitiveType(Shape::kPoints);
   shape->SetAttributeArray(attribute_array);

   NodePtr node(new Node);
   node->AddShape(shape);
   node->SetShaderProgram(shader);

   const ion::gfx::Uniform u = registry->Create<ion::gfx::Uniform>("uPointSize", 3.0f);

   node->AddUniform(u);

   m_WorldRoot->AddChild(node);

   //Finally, update the hud
}

void Scene::RenderFrame()
{
   GetRenderer()->DrawScene(GetRoot());
}

void Scene::BuildWorldSceneGraph()
{
   ion::gfxutils::EllipsoidSpec ellipsoid_spec;
   ellipsoid_spec.vertex_type = ion::gfxutils::ShapeSpec::kPosition;
   ellipsoid_spec.usage_mode = ion::gfx::BufferObject::kStaticDraw;
   ellipsoid_spec.translation = Point3f::Zero();
   ellipsoid_spec.size = Vector3f::Fill(1.0f);
   ellipsoid_spec.band_count = 10;
   ellipsoid_spec.sector_count = 10;
   ellipsoid_spec.longitude_start = ion::math::Anglef::FromDegrees(0);
   ellipsoid_spec.longitude_end = ion::math::Anglef::FromDegrees(360);
   ellipsoid_spec.latitude_start = ion::math::Anglef::FromDegrees(-90);
   ellipsoid_spec.latitude_end = ion::math::Anglef::FromDegrees(90);

   m_WorldRoot->AddShape(ion::gfxutils::BuildEllipsoidShape(ellipsoid_spec));

   m_WorldRoot->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<ion::gfx::Uniform>("uBaseColor", Vector4f(.222f, .4609f, .156f, 1.f)));
}

void Scene::BuildHudSceneGraph() {}
}
