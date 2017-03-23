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
#include "ion/base/datetime.h"
#include "FileManager.hpp"
#include "KeyboardHandler.hpp"

// Resources for the HUD.
ION_REGISTER_ASSETS(SnapshotAssets);

using namespace ion::gfx;
using namespace ion::math;
using namespace ion::base;
using namespace ion::text;

namespace Snapshot{

Scene::Scene(KeyboardHandler& keyboard):
   SceneBase(),
   m_FileManager(std::make_shared<FileManager>()),
   m_InputFiles(SETTINGS_INPUT_FILES, vector<string>(), "Sets the relative files to load into the snapshot tool"),

   m_EpochIndex(SETTINGS_INPUT_EPOCH_INDEX, 0, "Sets index of the input files to display"),
   m_HardBodyRadius(SETTINGS_CONFIG_HBR, .120f, "Sets the hard body radius between states in kilometers"),
   m_AllToAll_Use(SETTINGS_CONFIG_ALLTOALL_USE, false, "Determines whether to use One to One or All to All collisions"),
   m_AllToAll_BinCount(SETTINGS_CONFIG_ALLTOALL_BIN_COUNT, 1000000, "Specifies the max number of bins to use in the X/Y/Z direction combined when calculating All-to-ALL"),
   m_AllToAll_InitialBinMultiplier(SETTINGS_CONFIG_ALLTOALL_INITIAL_BIN_MULT, 1.0f, "If the All-to-All data falls outside the current bin boundary, the process must be restarted with a larger boundary. The initial value for the size of the boundary is determined by One-to-One boundary times this multiplier"),

   m_AllToAll_MaxBinTries(SETTINGS_CONFIG_ALLTOALL_MAX_BIN_TRIES, 5, "If the All-to-All data falls outside the current bin boundary, the boundary size will be doubled and the process is restarted. This specifies the maximum number of times to restart the process before terminating"),
   m_LookAtCOM(SETTINGS_SCENE_LOOKATCOM, false, "Sets the focus point to the COM of the cluster"),
   m_ShowCOM(SETTINGS_SCENE_SHOW_COM, false, "Shows a secondary axes at the COM of the cluster. The size of the axes will be the size of the cluser bounding box"),
   m_AutoScale(SETTINGS_SCENE_AUTOSCALE, false, "Enables autoscaling the axes to evenly display data"),
   m_PointSize(SETTINGS_SCENE_POINT_SIZE, 4.0f, "Specifies the size in pixels of each point"),
   m_MissDistColor(SETTINGS_SCENE_MISSDISTCOLOR, Vector4ui(45, 45, 48, 255), "Set the color of the minimum distance line in the scene. Enter values between 0 and 255 for each component to make up the RGBA color. For example, V[255, 255, 0, 255] makes a solid yellow color. Min dist line will change color next time the min dist value is changed")
{
   SnapshotAssets::RegisterAssetsOnce();

   using std::bind;
   using std::placeholders::_1;

   //Add listenr for setting
   m_InputFiles.RegisterListener("LoadInputFiles", [&](SettingBase* setting) { m_FileManager->SetFiles(static_cast<Setting<vector<string>>*>(setting)->GetValue()); });
   m_EpochIndex.RegisterListener("UpdateEpochIndex", [&](SettingBase* setting) { m_FileManager->SetEpochIndex(static_cast<Setting<uint32_t>*>(setting)->GetValue()); });

   m_HardBodyRadius.RegisterListener("UpdateHBR", [&](SettingBase* setting) { m_FileManager->SetHbr(static_cast<Setting<float>*>(setting)->GetValue()); });
   m_AllToAll_Use.RegisterListener("SetAllToAll", [&](SettingBase* setting) { m_FileManager->SetAllToAll(static_cast<Setting<bool>*>(setting)->GetValue()); });

   //m_LookAtCOM.RegisterListener("SetCenterOfMass", [&](SettingBase* setting) { ReloadPointData(); });
   //m_ShowCOM.RegisterListener("UpdateAutoscale", [&](SettingBase* setting) { ReloadPointData(); });
   //m_AutoScale.RegisterListener("UpdateAutoscale", [&](SettingBase* setting) { ReloadPointData(); });

   m_WorldRoot = BuildWorldSceneGraph();
   m_HudRoot = BuildHudSceneGraph();

   GetRoot()->AddChild(m_WorldRoot);
   GetRoot()->AddChild(m_HudRoot);

   m_FileManager->SetHud(m_Hud);

   keyboard.Initialize(m_FileManager, GetCamera());
}

Scene::~Scene() {}

bool Scene::Update(double elapsedTimeInSec, double secSinceLastFrame)
{
   //Occurs once per loop. May or may not draw the frame after

   //If return true, then we need to exit early
   if (!m_Hud->Update(elapsedTimeInSec, secSinceLastFrame))
      return false;

   SceneBase::Update(elapsedTimeInSec, secSinceLastFrame);

   auto pos = Vector3f((float)GetCamera()->PositionWorld()[0], (float)GetCamera()->PositionWorld()[1], (float)GetCamera()->PositionWorld()[2]);

   m_WorldRoot->SetUniformByName(UNIFORM_WORLD_CAMERAPOSITION, pos);

   return true;
}

void Scene::RenderFrame()
{
   GetRenderer()->DrawScene(GetRoot());
}

void buildAxesArrowAndTicks(ion::base::AllocVector<StateVertex> & data, const Vector3f & pDir, const Vector3f & sDir, const Vector4f & color)
{
   Vector4ui8 pColor = Vector4ui8(Vector4f::Fill(255.0f) * color);
   Vector4ui8 sColor = Vector4ui8(Vector4f(255.0f, 255.0f, 255.0f, 64.0f) * color);

   //Main
   data.push_back(StateVertex(pDir * Vector3f::Fill(0), pColor));
   data.push_back(StateVertex(pDir * Vector3f::Fill(1), pColor));

   //Arrow Head
   data.push_back(StateVertex(pDir * Vector3f::Fill(1), pColor));
   data.push_back(StateVertex(pDir * Vector3f::Fill(0.95f) + sDir * Vector3f::Fill(0.05f), pColor));
   data.push_back(StateVertex(pDir * Vector3f::Fill(1), pColor));
   data.push_back(StateVertex(pDir * Vector3f::Fill(0.95f) + sDir * Vector3f::Fill(-0.05f), pColor));

   //Tick Marks
   Vector3f sTick = sDir * Vector3f::Fill(0.025f);

   for (size_t i = 1; i < 10; i++)
   {
      if (i == 5)
      {
         //Double the tick size for the half marker
         data.push_back(StateVertex(pDir * Vector3f::Fill(0.1f * i) + 2.0f * sTick, sColor));
         data.push_back(StateVertex(pDir * Vector3f::Fill(0.1f * i) + -2.0f * sTick, sColor));
      }
      else
      {
         data.push_back(StateVertex(pDir * Vector3f::Fill(0.1f * i) + sTick, sColor));
         data.push_back(StateVertex(pDir * Vector3f::Fill(0.1f * i) + -sTick, sColor));
      }
   }
}

NodePtr Scene::BuildSimpleAxes(const ion::gfxutils::ShaderManagerPtr& shaderManager)
{
   //Now create the particles
   AttributeArrayPtr attribute_array(new AttributeArray);

   auto buffer = BufferObjectPtr(new BufferObject);

   SharedPtr<VectorDataContainer<StateVertex>> container = SharedPtr<VectorDataContainer<StateVertex>>(new VectorDataContainer<StateVertex>(false));

   auto& vector = *container->GetMutableVector();

   //X Axis
   buildAxesArrowAndTicks(vector, Vector3f(1, 0, 0), Vector3f(0, 1, 0), Vector4f(1, 0, 0, 1));
   buildAxesArrowAndTicks(vector, Vector3f(-1, 0, 0), Vector3f(0, 1, 0), Vector4f(1, 0, 0, 1));

   //Y Axis
   buildAxesArrowAndTicks(vector, Vector3f(0, 1, 0), Vector3f(0, 0, 1), Vector4f(0, 1, 0, 1));
   buildAxesArrowAndTicks(vector, Vector3f(0, -1, 0), Vector3f(0, 0, 1), Vector4f(0, 1, 0, 1));

   //Z Axis
   buildAxesArrowAndTicks(vector, Vector3f(0, 0, 1), Vector3f(1, 0, 0), Vector4f(0, 0, 1, 1));
   buildAxesArrowAndTicks(vector, Vector3f(0, 0, -1), Vector3f(1, 0, 0), Vector4f(0, 0, 1, 1));

   buffer->SetData(container, sizeof(StateVertex), container->GetVector().size(), BufferObject::kStaticDraw);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, ATTRIBUTE_GLOBAL_VERTEX)
      .BindAndNormalize(v.Color, ATTRIBUTE_GLOBAL_COLOR)
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

   FontImagePtr font_image = GetFontManager()->GetCachedFontImage("Axes");

   if (!font_image.Get())
   {
      GlyphSet glyph_set(AllocatorPtr(NULL));
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
   region.horizontal_alignment = kAlignLeft;

   OutlineBuilderPtr vBuilder(new OutlineBuilder(font_image, GetShaderManager(), AllocatorPtr()));

   if (vBuilder->Build(font_image->GetFont()->BuildLayout("V", region), BufferObject::kStaticDraw))
   {
      //Must set the color after building
      vBuilder->SetTextColor(Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
      vBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      vBuilder->SetOutlineWidth(1.0f);

      auto tNode = vBuilder->GetNode();

      node->AddChild(tNode);
   }

   OutlineBuilderPtr nBuilder(new OutlineBuilder(font_image, GetShaderManager(), AllocatorPtr()));

   if (nBuilder->Build(font_image->GetFont()->BuildLayout("N", region), BufferObject::kStaticDraw))
   {
      //Must set the color after building
      nBuilder->SetTextColor(Vector4f(0.0f, 1.0f, 0.0f, 1.0f));
      nBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      nBuilder->SetOutlineWidth(1.0f);

      auto tNode = nBuilder->GetNode();

      auto mat = RotationMatrixAxisAngleH(Vector3f(-1.0f, 0.0f, 0.0f), Anglef::FromDegrees(-90.0f));
      mat = RotationMatrixAxisAngleH(Vector3f(0.0f, 0.0f, 1.0f), Anglef::FromDegrees(90.0f)) * mat;

      tNode->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_MODELVIEWMATRIX, mat));

      node->AddChild(tNode);
   }

   OutlineBuilderPtr bBuilder(new OutlineBuilder(font_image, GetShaderManager(), AllocatorPtr()));

   if (bBuilder->Build(font_image->GetFont()->BuildLayout("B", region), BufferObject::kStaticDraw))
   {
      //Must set the color after building
      bBuilder->SetTextColor(Vector4f(0.0f, 0.0f, 1.0f, 1.0f));
      bBuilder->SetOutlineColor(Vector4f(0.2f, 0.2f, 0.2f, 1.0f));
      bBuilder->SetOutlineWidth(1.0f);

      auto tNode = bBuilder->GetNode();

      auto mat = RotationMatrixAxisAngleH(Vector3f(-1.0f, 0.0f, 0.0f), Anglef::FromDegrees(90.0f));
      mat = RotationMatrixAxisAngleH(Vector3f(0.0f, 1.0f, 0.0f), Anglef::FromDegrees(-90.0f)) * mat;

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

   auto modelMat = ScaleMatrixH(Vector3f::Fill(2.0f * m_HardBodyRadius.GetValue()));

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
                                        float newHBR = dynamic_cast<Setting<float>*>(setting)->GetValue();

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
   world->AddChild(BuildPointsNode());
   world->AddChild(BuildMissDistNode());

   world->AddChild(BuildSimpleAxes(GetShaderManager()));

   return world;
}

#define JD_GSFC_BASE 2400000.5    //Jan 05 1941 12:00:00.000 UTC

NodePtr Scene::BuildHudSceneGraph()
{
   m_Hud = std::make_shared<Hud>(GetFontManager(), GetShaderManager(), GetCamera()->GetViewportUniforms());

   m_Hud->AddHudItem(std::make_shared<HudItem>("BFMC Snapshot"));

   //Epoch
   auto epoch = std::make_shared<HudItem>("@ 00/00/0000 00:00:00.000");
   m_FileManager->AddPostProcessStep("Epoch", [=](const SnapshotData& snapshot)
                                     {
                                        double work = snapshot.GetEpoch();
                                        double workDay = floor(work);
                                        double fractionOfDay = work - workDay;
                                        double julianDay = floor(work + JD_GSFC_BASE);

                                        long l, n, i, j;

                                        l = static_cast<long>(floor(julianDay)) + 68569l;
                                        n = 4l * l / 146097l;
                                        l = l - (146097l * n + 3l) / 4l;
                                        i = 4000l * (l + 1l) / 1461001l;
                                        l = l - 1461l * i / 4l + 31l;
                                        j = 80l * l / 2447l;
                                        uint8_t day = static_cast<uint8_t>(l - 2447l * j / 80l + 1); //Add one for index starting at 1 not 0
                                        l = j / 11l;
                                        uint8_t month = static_cast<uint8_t>(j + 2l - 12l * l);
                                        int64_t year = 100l * (n - 49l) + i + l;

                                        int32_t seconds = static_cast<int32_t>(abs(fractionOfDay * 86400.0));
                                        int32_t nanoseconds = static_cast<int32_t>(fractionOfDay * 86400000.0 - seconds * 1000.0 + 0.5) * 1000000;

                                        uint8_t hours = static_cast<uint8_t>(seconds / 3600);

                                        seconds -= static_cast<int32_t>(hours) * 3600;

                                        uint8_t minutes = static_cast<int32_t>(seconds / 60);

                                        seconds -= static_cast<int32_t>(minutes) * 60;

                                        DateTime fromMJD(year, month, day, hours, minutes, static_cast<uint8_t>(seconds), nanoseconds);

                                        std::string epochText = "@ ";
                                        std::string temp = "";

                                        fromMJD.ComputeDateString(DateTime::DateStringEnum::kRenderDayMonthYear, &temp);
                                        epochText += temp;

                                        fromMJD.ComputeTimeString(DateTime::TimeStringEnum::kRenderHoursMinutesSeconds, &temp);
                                        epochText += temp;

                                        epoch->SetText(epochText);
                                     });
   m_Hud->AddHudItem(epoch);

   //Pc
   auto pc = std::make_shared<HudItem>("Pc: 0.0");
   m_FileManager->AddPostProcessStep("Pc", [=](const SnapshotData& snapshot)
                                     {
                                        pc->SetText("Pc: " + ion::base::ValueToString(snapshot.GetStats().Pc));
                                     });
   m_Hud->AddHudItem(pc);

   //Min Dist
   auto minDist = std::make_shared<HudItem>("Min Dist (km): 0.0");
   m_FileManager->AddPostProcessStep("MinDist", [=](const SnapshotData& snapshot)
                                     {
                                        minDist->SetText("Min Dist (km): " + ion::base::ValueToString(Length(Vector3f::ToVector(snapshot.GetStats().MinMiss))));
                                     });
   m_Hud->AddHudItem(minDist);

   //HBR
   auto hbr = std::make_shared<HudItem>("HBR (km): 0.120");
   m_HardBodyRadius.RegisterListener("HBR", [=](SettingBase* setting)
                                     {
                                        hbr->SetText("HBR (km): " + ion::base::ValueToString(static_cast<Setting<float>*>(setting)->GetValue()));
                                     });
   m_Hud->AddHudItem(hbr);

   //Collision Type
   auto allToAll = std::make_shared<HudItem>("One-to-One");
   m_AllToAll_Use.RegisterListener("UpdateHUD", [=](SettingBase* setting)
                                   {
                                      allToAll->SetText(static_cast<Setting<bool>*>(setting)->GetValue() ? "All-to-All" : "One-to-One");
                                   });
   m_Hud->AddHudItem(allToAll);

   m_Hud->GetRootNode()->SetLabel("HUD");

   return m_Hud->GetRootNode();
}

ion::gfx::NodePtr Scene::BuildPointsNode()
{
   AttributeArrayPtr attribute_array(new AttributeArray);
   ShaderInputRegistryPtr pointsRegistry = ShaderInputRegistryPtr(new ShaderInputRegistry);
   pointsRegistry->IncludeGlobalRegistry();

   pointsRegistry->Add(ShaderInputRegistry::UniformSpec("uPointSize", kFloatUniform, "The size in pixels of the point"));

   auto pointsBuffer = BufferObjectPtr(new BufferObject);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, ATTRIBUTE_GLOBAL_VERTEX)
      .BindAndNormalize(v.Color, ATTRIBUTE_GLOBAL_COLOR)
      .Apply(pointsRegistry, attribute_array, pointsBuffer);

   ShaderProgramPtr pointsShader = GetShaderManager()->CreateShaderProgram("Point",
                                                                           pointsRegistry,
                                                                           ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Point.vert", false)),
                                                                           ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Point.frag", false)));

   ShapePtr shape(new Shape);
   shape->SetPrimitiveType(Shape::kPoints);
   shape->SetAttributeArray(attribute_array);

   auto pointsNode = NodePtr(new Node);
   pointsNode->AddShape(shape);
   pointsNode->SetShaderProgram(pointsShader);

   pointsNode->AddUniform(pointsRegistry->Create<Uniform>("uPointSize", m_PointSize.GetValue()));

   m_PointSize.RegisterListener("UpdateAutoscale", [=](SettingBase* setting)
                                {
                                   pointsNode->AddUniform(pointsRegistry->Create<Uniform>("uPointSize", static_cast<Setting<float>*>(setting)->GetValue()));
                                });

   pointsNode->Enable(false);

   m_FileManager->AddPostProcessStep("SetBufferData", [=](const SnapshotData& snapshot)
                                     {
                                        SharedPtr<VectorDataContainer<StateVertex>> vertexData(new VectorDataContainer<StateVertex>(true));

                                        vertexData->GetMutableVector()->insert(vertexData->GetMutableVector()->end(), snapshot.GetOutputData().begin(), snapshot.GetOutputData().end());

                                        pointsBuffer->SetData(vertexData, sizeof(StateVertex), vertexData->GetVector().size(), BufferObject::kStaticDraw);

                                        pointsNode->Enable(snapshot.GetOutputData().size() > 0);
                                     });

   return pointsNode;
}

ion::gfx::NodePtr Scene::BuildMissDistNode()
{
   //Now create the particles
   AttributeArrayPtr attribute_array(new AttributeArray);

   auto buffer = BufferObjectPtr(new BufferObject);

   SharedPtr<VectorDataContainer<StateVertex>> container = SharedPtr<VectorDataContainer<StateVertex>>(new VectorDataContainer<StateVertex>(false));

   auto& vector = *container->GetMutableVector();

   vector.push_back(StateVertex(Vector3f(0, 0, 0), Vector4ui8(255, 0, 0, 255)));
   vector.push_back(StateVertex(Vector3f(1, 0, 0), Vector4ui8(255, 0, 0, 255)));

   buffer->SetData(container, sizeof(StateVertex), container->GetVector().size(), BufferObject::kStaticDraw);

   StateVertex v;
   ion::gfxutils::BufferToAttributeBinder<StateVertex>(v)
      .Bind(v.Pos, ATTRIBUTE_GLOBAL_VERTEX)
      .BindAndNormalize(v.Color, ATTRIBUTE_GLOBAL_COLOR)
      .Apply(ShaderInputRegistry::GetGlobalRegistry(), attribute_array, buffer);

   ShapePtr shape(new Shape);
   shape->SetPrimitiveType(Shape::kLines);
   shape->SetAttributeArray(attribute_array);

   ShaderProgramPtr flatShader = GetShaderManager()->CreateShaderProgram("Flat",
                                                                         ShaderInputRegistry::GetGlobalRegistry(),
                                                                         ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Flat.vert", false)),
                                                                         ion::gfxutils::ShaderSourceComposerPtr(new ion::gfxutils::ZipAssetComposer("Flat.frag", false)));

   auto node = NodePtr(new Node);
   node->SetLabel("Miss Distance");

   StateTablePtr stateTable(new StateTable());
   stateTable->SetLineWidth(3.0f);

   node->SetStateTable(stateTable);

   node->AddShape(shape);
   node->SetShaderProgram(flatShader);

   node->AddUniform(ShaderInputRegistry::GetGlobalRegistry()->Create<Uniform>(UNIFORM_GLOBAL_BASECOLOR, Vector4f(1.0f, 1.0f, 1.0f, 1.0f)));
   //node->Enable(false);

   m_MissDistColor.RegisterListener("Bound color", [](SettingBase* setting) {

      auto colorSetting = static_cast<Setting<Vector4ui>*>(setting);

      Vector4ui clamped = colorSetting->GetValue();

      for (size_t i = 0; i < 4; i++)
      {
         clamped[i] = std::min(clamped[i], 255u);
      }

      if (clamped != colorSetting->GetValue())
         colorSetting->SetValue(clamped);
   });

   m_FileManager->AddPostProcessStep("SetMinMissData", [=](const SnapshotData& snapshot)
                                     {
                                        SharedPtr<VectorDataContainer<StateVertex>> vertexData(new VectorDataContainer<StateVertex>(true));

                                        Vector3f endPoint = Vector3f::ToVector(snapshot.GetStats().MinMiss);
                                        
                                        //Guarenteed to be between 0-255
                                        Vector4ui8 primaryColor = Vector4ui8(m_MissDistColor.GetValue());
                                        Vector4ui8 secondaryColor = primaryColor;
                                        secondaryColor[3] = primaryColor[3] / 3;

                                        //Create the primary line
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(0, 0, 0), primaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(endPoint, primaryColor));

                                        //Projection into VN
                                        Vector3f projPoint = Vector3f(endPoint[0], endPoint[1], 0.0f);

                                        //Now add the projection of the vector onto the planes
                                        vertexData->GetMutableVector()->push_back(StateVertex(endPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(0.0f, projPoint[1], projPoint[2]), secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(projPoint[0], 0.0f, projPoint[2]), secondaryColor));

                                        //Projection into VB
                                        projPoint = Vector3f(endPoint[0], 0.0f, endPoint[2]);

                                        //Now add the projection of the vector onto the planes
                                        vertexData->GetMutableVector()->push_back(StateVertex(endPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(0.0f, projPoint[1], projPoint[2]), secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(projPoint[0], projPoint[1], 0.0f), secondaryColor));

                                        //Projection into NB
                                        projPoint = Vector3f(0.0f, endPoint[1], endPoint[2]);

                                        //Now add the projection of the vector onto the planes
                                        vertexData->GetMutableVector()->push_back(StateVertex(endPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(projPoint[0], 0.0f, projPoint[2]), secondaryColor));

                                        vertexData->GetMutableVector()->push_back(StateVertex(projPoint, secondaryColor));
                                        vertexData->GetMutableVector()->push_back(StateVertex(Vector3f(projPoint[0], projPoint[1], 0.0f), secondaryColor));

                                        buffer->SetData(vertexData, sizeof(StateVertex), vertexData->GetVector().size(), BufferObject::kStaticDraw);

                                        node->Enable(snapshot.GetOutputData().size() > 0);
                                     });

   return node;
}
}
