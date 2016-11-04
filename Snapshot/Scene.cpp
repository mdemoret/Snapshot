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
#include "ion/math/transformutils.h"
#include "ion/math/matrixutils.h"
#include "ion/demos/utils.h"
#include "ion/text/outlinebuilder.h"

// Resources for the HUD.
ION_REGISTER_ASSETS(SnapshotAssets);

using namespace ion::gfx;
using namespace ion::math;
using namespace ion::base;
using namespace ion::text;

namespace Snapshot{

Scene::Scene():
   SceneBase(),
   m_InputFiles(SETTINGS_INPUT_FILES, vector<string>(), "Sets the relative files to load into the snapshot tool"),
   m_HardBodyRadius(SETTINGS_HBR, 120.0f, "Sets the hard body radius between states in meters"),
   m_EpochIndex(SETTINGS_EPOCH_INDEX, 0, "Sets index of the input files to display"),
   m_LookAtCOM(SETTINGS_SCENE_LOOKATCOM, false, "Sets the focus point to the COM of the cluster"),
   m_AutoScale(SETTINGS_SCENE_AUTOSCALE, false, "Enables autoscaling the axes to evenly display data"),
   m_UseAllToAll(SETTINGS_SCENE_ALLTOALL, false, "Determines whether to use One to One or All to All collisions")
{
   SnapshotAssets::RegisterAssetsOnce();

   using std::bind;
   using std::placeholders::_1;

   //Add listenr for setting
   m_InputFiles.RegisterListener("LoadInputFiles", bind(&Scene::LoadInputFiles, this, _1));
   m_HardBodyRadius.RegisterListener("UpdateHBR", [&](SettingBase* setting) { ReloadPointData(); });
   m_LookAtCOM.RegisterListener("SetCenterOfMass", [&](SettingBase* setting) { ReloadPointData(); });
   m_AutoScale.RegisterListener("UpdateAutoscale", [&](SettingBase* setting) { ReloadPointData(); });
   m_UseAllToAll.RegisterListener("SetAllToAll", [&](SettingBase* setting) { ReloadPointData(); });

   m_WorldRoot = BuildWorldSceneGraph();
   m_HudRoot = BuildHudSceneGraph();

   GetRoot()->AddChild(m_WorldRoot);
   GetRoot()->AddChild(m_HudRoot);
}

Scene::~Scene() {}

void Scene::Update()
{
   SceneBase::Update();

   auto pos = Vector3f((float)GetCamera()->PositionWorld()[0], (float)GetCamera()->PositionWorld()[1], (float)GetCamera()->PositionWorld()[2]);

   m_WorldRoot->SetUniformByName(UNIFORM_WORLD_CAMERAPOSITION, pos);

   //Occurs once per loop. May or may not draw the frame after
   m_Hud->Update();
}

void Scene::LoadInputFiles(SettingBase* setting)
{
   //First attempt to read the files
   if (!m_FileManager.LoadFiles(static_cast<Setting<vector<string>>*>(setting)->GetValue()))
   {
      //Error
      LOG(ERROR) << "Error reading files";
      return;
   }

   m_PointsNode->Enable(true);

   ReloadPointData();
}

void Scene::ReloadPointData()
{
   Range3f bounds;

   auto container = m_FileManager.GetData(m_EpochIndex.GetValue(), 1, 0, bounds);

   m_PointsBuffer->SetData(container, sizeof(StateVertex), container->GetVector().size(), BufferObject::kStaticDraw);

   if (m_AutoScale.GetValue())
   {
      auto size = bounds.GetSize();

      float maxVal = max(size[0], max(size[1], size[2]));

      //If we want to scale this, apply that here
      m_WorldRoot->SetUniformByName(UNIFORM_GLOBAL_MODELVIEWMATRIX, ScaleMatrixH(maxVal / bounds.GetSize()));
   }

   auto center = Point3d::Zero();

   if (m_LookAtCOM.GetValue())
      center = static_cast<Point3d>(bounds.GetCenter());

   GetCamera()->SetLookAt(center);
}

void Scene::RenderFrame()
{
   GetRenderer()->DrawScene(GetRoot());
}

NodePtr Scene::BuildSimpleAxes(const ion::gfxutils::ShaderManagerPtr& shaderManager)
{
   //Now create the particles
   AttributeArrayPtr attribute_array(new AttributeArray);

   auto buffer = BufferObjectPtr(new BufferObject);

   SharedPtr<VectorDataContainer<StateVertex>> container = SharedPtr<VectorDataContainer<StateVertex>>(new VectorDataContainer<StateVertex>(false));

   auto& vector = *container->GetMutableVector();

   vector.push_back(StateVertex(Vector3f(0, 0, 0), Vector4ui8(255, 0, 0, 255)));
   vector.push_back(StateVertex(Vector3f(1, 0, 0), Vector4ui8(255, 0, 0, 255)));

   vector.push_back(StateVertex(Vector3f(0, 0, 0), Vector4ui8(0, 255, 0, 255)));
   vector.push_back(StateVertex(Vector3f(0, 1, 0), Vector4ui8(0, 255, 0, 255)));

   vector.push_back(StateVertex(Vector3f(0, 0, 0), Vector4ui8(0, 0, 255, 255)));
   vector.push_back(StateVertex(Vector3f(0, 0, 1), Vector4ui8(0, 0, 255, 255)));


   buffer->SetData(container, sizeof(StateVertex), container->GetVector().size(), BufferObject::kStaticDraw);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, ATTRIBUTE_GLOBAL_VERTEX)
      .Bind(v.Color, ATTRIBUTE_GLOBAL_COLOR)
      .Apply(ShaderInputRegistry::GetGlobalRegistry(), attribute_array, buffer);

   ShapePtr shape(new Shape);
   shape->SetPrimitiveType(Shape::kLines);
   shape->SetAttributeArray(attribute_array);

   ShaderProgramPtr flatShader = shaderManager->CreateShaderProgram("Flat",
                                                                    ShaderInputRegistry::GetGlobalRegistry(),
                                                                    ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Flat.vert", false)),
                                                                    ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Flat.frag", false)));

   auto node = NodePtr(new Node);
   node->SetLabel("Axes");

   StateTablePtr stateTable(new StateTable());
   stateTable->SetLineWidth(2.5f);

   node->SetStateTable(stateTable);

   node->AddShape(shape);
   node->SetShaderProgram(flatShader);

   node->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_BASECOLOR, Vector4f(1.0f, 1.0f, 1.0f, 1.0f)));
   //node->Enable(false);

   //Now add the text nodes
   auto font = demoutils::InitFont(GetFontManager(), "Hud", 20U, 8U);

   ion::text::FontImagePtr font_image = GetFontManager()->GetCachedFontImage("Axes");

   if (!font_image.Get())
   {
      ion::text::GlyphSet glyph_set(ion::base::AllocatorPtr(NULL));
      font->AddGlyphsForAsciiCharacterRange('V', 'V', &glyph_set);
      font->AddGlyphsForAsciiCharacterRange('N', 'N', &glyph_set);
      font->AddGlyphsForAsciiCharacterRange('B', 'B', &glyph_set);

      StaticFontImagePtr sfi(new StaticFontImage(font, 256U, glyph_set));
      if (!sfi->GetImageData().texture.Get())
      {
         LOG(ERROR) << "Unable to create HUD FontImage";
      }
      else
      {
         GetFontManager()->CacheFontImage("Axes", sfi);
         font_image = sfi;
      }
   }

   LayoutOptions region;
   region.target_point = Point2f(1.0f, 0.02f);
   region.target_size = Vector2f(0.0f, 0.1f);
   region.horizontal_alignment = ion::text::HorizontalAlignment::kAlignRight;

   OutlineBuilderPtr vBuilder(new OutlineBuilder(font_image, GetShaderManager(), ion::base::AllocatorPtr()));

   if (vBuilder->Build(font_image->GetFont()->BuildLayout("V", region), ion::gfx::BufferObject::kStaticDraw))
   {
      //Must set the color after building
      vBuilder->SetTextColor(Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
      vBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      vBuilder->SetOutlineWidth(1.0f);

      auto tNode = vBuilder->GetNode();

      node->AddChild(tNode);
   }

   OutlineBuilderPtr nBuilder(new OutlineBuilder(font_image, GetShaderManager(), ion::base::AllocatorPtr()));

   if (nBuilder->Build(font_image->GetFont()->BuildLayout("N", region), ion::gfx::BufferObject::kStaticDraw))
   {
      //Must set the color after building
      nBuilder->SetTextColor(Vector4f(0.0f, 1.0f, 0.0f, 1.0f));
      nBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      nBuilder->SetOutlineWidth(1.0f);

      auto tNode = nBuilder->GetNode();

      auto mat = ion::math::RotationMatrixAxisAngleH(Vector3f(-1.0f, 0.0f, 0.0f), Anglef::FromDegrees(-90.0f));
      mat = ion::math::RotationMatrixAxisAngleH(Vector3f(0.0f, 0.0f, 1.0f), Anglef::FromDegrees(90.0f)) * mat;

      tNode->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, mat));

      node->AddChild(tNode);
   }

   OutlineBuilderPtr bBuilder(new OutlineBuilder(font_image, GetShaderManager(), ion::base::AllocatorPtr()));

   if (bBuilder->Build(font_image->GetFont()->BuildLayout("B", region), ion::gfx::BufferObject::kStaticDraw))
   {
      //Must set the color after building
      bBuilder->SetTextColor(Vector4f(0.0f, 0.0f, 1.0f, 1.0f));
      bBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      bBuilder->SetOutlineWidth(1.0f);

      auto tNode = bBuilder->GetNode();

      auto mat = ion::math::RotationMatrixAxisAngleH(Vector3f(-1.0f, 0.0f, 0.0f), Anglef::FromDegrees(90.0f));
      mat = ion::math::RotationMatrixAxisAngleH(Vector3f(0.0f, 1.0f, 0.0f), Anglef::FromDegrees(-90.0f)) * mat;

      tNode->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, mat));

      node->AddChild(tNode);
   }

   return node;
}

NodePtr Scene::BuildWorldSceneGraph()
{
   //Build Scene Graph
   //Initialize the 3d and Hud nodes
   auto world = NodePtr(new Node());
   world->SetLabel("World Root");

   ShaderInputRegistryPtr worldRegistry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   worldRegistry->IncludeGlobalRegistry();

   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_CAMERAPOSITION, kFloatVector3Uniform, "The position of the camera in world coordinates"));
   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_LIGHTDIRECTION, kFloatVector3Uniform, "The direction of the light in world coordinates"));
   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_LIGHTAMBIENT, kFloatVector3Uniform, "The light ambient color"));
   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_LIGHTDIFFUSE, kFloatVector3Uniform, "The light diffuse color"));
   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_LIGHTSPECULAR, kFloatVector3Uniform, "The light specular color"));
   worldRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_WORLD_LIGHTSPECULARINTENSITY, kFloatUniform, "The specular intensity of the light"));

   //Add the uniforms for lighting
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, Matrix4f::Identity()));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_CAMERAPOSITION, Vector3f::Zero()));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTDIRECTION, Vector3f(0.0f, 0.0f, 1.0f)));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTAMBIENT, Vector3f(0.05f, 0.05f, 0.05f)));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTDIFFUSE, Vector3f(1.0f, 1.0f, 1.0f)));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTSPECULAR, Vector3f(0.25f, 0.25f, 0.25f)));
   world->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTSPECULARINTENSITY, 32.0f));

   //Add the uniforms for 3d rotation
   world->AddUniformBlock(GetCamera()->Get3dUniforms());

   auto hbr = NodePtr(new Node());
   hbr->SetLabel("Hard Body Radius");

   auto solidHbr = NodePtr(new Node());
   solidHbr->SetLabel("Solid Hard Body Radius");

   auto wireHbr = NodePtr(new Node());
   wireHbr->SetLabel("Wireframe Hard Body Radius");

   hbr->AddChild(solidHbr);
   hbr->AddChild(wireHbr);

   ion::gfxutils::EllipsoidSpec ellipsoid_spec;
   ellipsoid_spec.vertex_type = ion::gfxutils::ShapeSpec::kPositionNormal;
   ellipsoid_spec.usage_mode = BufferObject::kStaticDraw;
   ellipsoid_spec.translation = Point3f::Zero();
   ellipsoid_spec.size = Vector3f::Fill(1.0f);
   ellipsoid_spec.band_count = 100;
   ellipsoid_spec.sector_count = 100;
   ellipsoid_spec.longitude_start = Anglef::FromDegrees(0);
   ellipsoid_spec.longitude_end = Anglef::FromDegrees(360);
   ellipsoid_spec.latitude_start = Anglef::FromDegrees(-90);
   ellipsoid_spec.latitude_end = Anglef::FromDegrees(90);

   solidHbr->AddShape(BuildEllipsoidShape(ellipsoid_spec));

   ellipsoid_spec.band_count = 36;
   ellipsoid_spec.sector_count = 18;
   //ellipsoid_spec.vertex_type = ion::gfxutils::ShapeSpec::kPosition;

   auto wireframeHBRShape = BuildEllipsoidShape(ellipsoid_spec);
   //wireframeHBR->SetAttributeArray(tri_shape->GetAttributeArray());

   // Draw as lines.
   wireframeHBRShape->SetPrimitiveType(Shape::kLines);

   // Modify the index buffer to convert triangles to lines.
   wireframeHBRShape->SetIndexBuffer(ion::gfxutils::BuildWireframeIndexBuffer(wireframeHBRShape->GetIndexBuffer()));

   wireHbr->AddShape(wireframeHBRShape);

   ShaderInputRegistryPtr phongRegistry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   phongRegistry->Include(worldRegistry);

   phongRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_COMMON_MODELMATRIX, kMatrix4x4Uniform, "The model matrix transformation for the object"));
   phongRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_COMMON_NORMALMATRIX, kMatrix3x3Uniform, "The normal matrix transformation for the object"));

   ShaderProgramPtr phongShader = GetShaderManager()->CreateShaderProgram("Phong",
                                                                          phongRegistry,
                                                                          ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Phong.vert", false)),
                                                                          ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Phong.frag", false)));

   hbr->SetShaderProgram(phongShader);

   auto modelMat = ScaleMatrixH(Vector3f::Fill(2.0f * m_HardBodyRadius.GetValue() / 1000.0f));

   auto normalMat = Transpose(Inverse(NonhomogeneousSubmatrixH(modelMat)));

   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_GLOBAL_BASECOLOR, Vector4f(.222f, .4609f, .156f, 0.5f)));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, modelMat));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_COMMON_MODELMATRIX, modelMat));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_COMMON_NORMALMATRIX, normalMat));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTAMBIENT, Vector3f(.8f, .8f, .8f)));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTDIFFUSE, Vector3f(.5f, .5f, .5f)));
   hbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTSPECULAR, Vector3f(.1f, .1f, .1f)));
   hbr->AddUniform(worldRegistry->Create<Uniform>(UNIFORM_WORLD_LIGHTSPECULARINTENSITY, 16.0f));

   m_HardBodyRadius.RegisterListener("UpdateHBRShape", [hbr](SettingBase* setting)
                                     {
                                        float newHBR = dynamic_cast<Setting<float>*>(setting)->GetValue() / 1000.0f;

                                        auto modelMat = ScaleMatrixH(Vector3f::Fill(2.0f * newHBR));

                                        auto normalMat = Transpose(Inverse(NonhomogeneousSubmatrixH(modelMat)));

                                        hbr->SetUniformByName(UNIFORM_GLOBAL_MODELVIEWMATRIX, modelMat);
                                        hbr->SetUniformByName(UNIFORM_COMMON_MODELMATRIX, modelMat);
                                        hbr->SetUniformByName(UNIFORM_COMMON_NORMALMATRIX, normalMat);
                                     });

   //ShaderInputRegistryPtr wireframeRegistry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   //wireframeRegistry->Include(phongRegistry);

   //wireframeRegistry->Add(ShaderInputRegistry::UniformSpec(UNIFORM_GLOBAL_BASECOLOR, kFloatVector4Uniform, "The model matrix transformation for the object", [&](const Uniform & oldValue, const Uniform & newValue) {
   //   const VectorBase4f & c0 = oldValue.GetValue<VectorBase4f>();
   //   const VectorBase4f & c1 = newValue.GetValue<VectorBase4f>();

   //   //Override the alpha channel
   //   Uniform result = oldValue;

   //   result.SetValue(Vector4f(c0[0], c0[1], c0[2], c1[3]));
   //   return result;
   //}));

   wireHbr->SetShaderProgram(phongShader);

   wireHbr->AddUniform(phongRegistry->Create<Uniform>(UNIFORM_GLOBAL_BASECOLOR, Vector4f::Fill(0.1f) + Vector4f(.222f, .4609f, .156f, 0.5f)));


   StateTablePtr hbrSolidTable(new StateTable());
   hbrSolidTable->SetDepthWriteMask(false);

   solidHbr->SetStateTable(hbrSolidTable);
   //solidHbr->Enable(false);

   world->AddChild(hbr);

   //Now create the particles
   AttributeArrayPtr attribute_array(new AttributeArray);
   ShaderInputRegistryPtr pointsRegistry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   pointsRegistry->IncludeGlobalRegistry();

   pointsRegistry->Add(ShaderInputRegistry::UniformSpec("uPointSize", kFloatUniform, "The size in pixels of the point"));

   m_PointsBuffer = BufferObjectPtr(new BufferObject);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, ATTRIBUTE_GLOBAL_VERTEX)
      .BindAndNormalize(v.Color, ATTRIBUTE_GLOBAL_COLOR)
      .Apply(pointsRegistry, attribute_array, m_PointsBuffer);

   ShaderProgramPtr pointsShader = GetShaderManager()->CreateShaderProgram("Point",
                                                                           pointsRegistry,
                                                                           ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Point.vert", false)),
                                                                           ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Point.frag", false)));

   ShapePtr shape(new Shape);
   shape->SetPrimitiveType(Shape::kPoints);
   shape->SetAttributeArray(attribute_array);

   m_PointsNode = NodePtr(new Node);
   m_PointsNode->AddShape(shape);
   m_PointsNode->SetShaderProgram(pointsShader);

   m_PointsNode->AddUniform(pointsRegistry->Create<Uniform>("uPointSize", 3.0f));
   m_PointsNode->Enable(false);

   //StateTablePtr pointsTable(new StateTable());
   //pointsTable->Enable(StateTable::kMultisample, false);

   //m_PointsNode->SetStateTable(pointsTable);

   world->AddChild(m_PointsNode);

   world->AddChild(BuildSimpleAxes(GetShaderManager()));

   return world;
}

NodePtr Scene::BuildHudSceneGraph()
{
   m_Hud = std::make_shared<Hud>(GetFontManager(), GetShaderManager(), GetCamera()->GetViewportUniforms());

   m_Hud->AddHudItem(std::make_shared<HudItem>("BFMC Snapshot"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("@ 12/09/2016 08:58:27.379"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("Pc = 5.4e-04"));
   m_Hud->AddHudItem(std::make_shared<HudItem>("HBR = 120 m"));

   m_Hud->GetRootNode()->SetLabel("HUD");

   return m_Hud->GetRootNode();
}
}
