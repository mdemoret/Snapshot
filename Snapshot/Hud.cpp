#include "Hud.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

#include "ion/base/invalid.h"
#include "ion/base/logging.h"
#include "ion/base/zipassetmanager.h"
#include "ion/base/zipassetmanagermacros.h"
#include "ion/demos/utils.h"
#include "ion/gfx/bufferobject.h"
#include "ion/gfx/shaderinputregistry.h"
#include "ion/gfx/statetable.h"
#include "ion/gfxutils/shadermanager.h"
#include "ion/math/matrix.h"
#include "ion/math/range.h"
#include "ion/math/transformutils.h"
#include "ion/port/timer.h"
#include "ion/text/basicbuilder.h"
#include "ion/text/font.h"
#include "ion/text/fontimage.h"
#include "ion/text/layout.h"
#include "Macros.h"
#include "ion/text/freetypefontutils.h"
#include "ion/base/stringutils.h"
#include "ion/text/freetypefont.h"

// Resources for the HUD.
ION_REGISTER_ASSETS(SnapshotAssets);


using namespace ion::text;
using namespace ion::math;
//-----------------------------------------------------------------------------
//
// Helper functions.
//
//-----------------------------------------------------------------------------

// Builds the root node for the HUD.
static ion::gfx::NodePtr BuildHudRootNode(int width, int height)
{
   ion::gfx::NodePtr node(new ion::gfx::Node);

   // Set an orthographic projection matrix and identity modelview matrix.
   const ion::gfx::ShaderInputRegistryPtr& global_reg = ion::gfx::ShaderInputRegistry::GetGlobalRegistry();
   const Matrix4f proj = OrthographicMatrixFromFrustum(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
   demoutils::AddUniformToNode(global_reg, "uProjectionMatrix", proj, node);
   demoutils::AddUniformToNode(global_reg, "uModelviewMatrix", Matrix4f::Identity(), node);

   return node;
}

FontPtr Hud::InitFont(const string& font_name, size_t size_in_pixels, size_t sdf_padding) const
{
   return demoutils::InitFont(m_FontManager, font_name, size_in_pixels, sdf_padding);
}

const FontImagePtr Hud::InitFontImage(const string& key, const FontPtr& font) const
{
   FontImagePtr font_image = m_FontManager->GetCachedFontImage(key);
   if (!font_image.Get())
   {
      DynamicFontImagePtr dfi(new DynamicFontImage(font, 256U));
      if (!dfi->GetImageData(0).texture.Get())
      {
         LOG(ERROR) << "Unable to create HUD FontImage";
      }
      else
      {
         m_FontManager->CacheFontImage(key, dfi);
         font_image = dfi;
      }
   }
   return font_image;
}

size_t Hud::AddText(const FontImagePtr& font_image, const LayoutOptions& region, const string& text)
{
   size_t id = ion::base::kInvalidIndex;
   if (font_image.Get()) { }
   return id;
}

HudItem::HudItem(const string& startString):
m_Text(startString){}

void HudItem::SetText(const string& text) 
{
   m_Text = text;
}

string HudItem::GetText() const
{
   return m_Text;
}

ProgressHandler::~ProgressHandler() {
//As we close, set the handle back to null
   m_Parent->m_GetProgressFunc = nullptr;
}

void ProgressHandler::SetProgressFunc(const std::function<string()>& getProgress)
{
   if (getProgress == nullptr)
      m_Parent->m_GetProgressFunc = DefaultHandler;
   else
      m_Parent->m_GetProgressFunc = getProgress;
}

ProgressHandler::ProgressHandler(ProgressHudItem* parent) :
   m_Parent(parent)
{
   //Begin by setting the parent processing handle to the default
   m_Parent->m_GetProgressFunc = DefaultHandler;
}

string ProgressHandler::DefaultHandler() {
   return "Loading";
}

ProgressHudItem::~ProgressHudItem() {}

ProgressHandler ProgressHudItem::GetProgressHandler() {
   return ProgressHandler(this);
}

bool ProgressHudItem::IsProcessingActive() const 
{
   return m_GetProgressFunc != nullptr;
}

string ProgressHudItem::GetText() const 
{
   if (!IsProcessingActive() || m_GetProgressFunc == nullptr)
      return "";

   return m_GetProgressFunc();
}
//-----------------------------------------------------------------------------
//
// Hud class functions.
//
//-----------------------------------------------------------------------------
Hud::Hud(const FontManagerPtr& font_manager, const ion::gfxutils::ShaderManagerPtr& shader_manager, const ion::gfx::UniformBlockPtr& viewportUniforms):
   m_Root(BuildHudRootNode(1, 1)),
   m_FontManager(font_manager),
   m_ShaderManager(shader_manager),
   m_ViewportUniforms(viewportUniforms)
{
   SnapshotAssets::RegisterAssetsOnce();

   //Set up the progress handler
   FontPtr font = InitFont("Hud", 30U, 8U);
   if (font.Get())
   {
      FontImagePtr font_image(InitFontImage("HUD Progress", font));

      LayoutOptions region;
      region.horizontal_alignment = kAlignHCenter;
      region.vertical_alignment = kAlignVCenter;

      string text = "Loading";

      BasicBuilderPtr builder(new BasicBuilder(font_image, m_ShaderManager, ion::base::AllocatorPtr()));

      const Layout layout = font_image->GetFont()->BuildLayout(text, region);

      if (builder->Build(layout, ion::gfx::BufferObject::kStreamDraw))
      {
         //Must set the color after building
         builder->SetTextColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

         TextSpec text_spec;
         text_spec.region = region;
         text_spec.text = text;
         text_spec.builder = builder;
         text_spec.node = builder->GetNode();

         m_ProgressItem = std::pair<ProgressHudItemPtr, TextSpec>(std::make_shared<ProgressHudItem>(""), text_spec);

         m_Root->AddChild(text_spec.node);
      }
   }
}

Hud::~Hud() {}

ProgressHudItemPtr Hud::GetProgressHudItem() const 
{
   return m_ProgressItem.first;
}

size_t Hud::AddHudItem(const HudItemPtr& item)
{
   FontPtr font = InitFont("Hud", 20U, 8U);
   if (font.Get())
   {
      FontImagePtr font_image(InitFontImage("HUD FPS", font));

      LayoutOptions region;

      string text = item->GetText();

      BasicBuilderPtr builder(new BasicBuilder(font_image, m_ShaderManager, ion::base::AllocatorPtr()));

      const Layout layout = font_image->GetFont()->BuildLayout(text, region);

      if (builder->Build(layout, ion::gfx::BufferObject::kStreamDraw))
      {
         //Must set the color after building
         builder->SetTextColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

         TextSpec text_spec;
         text_spec.region = region;
         text_spec.text = text;
         text_spec.builder = builder;
         text_spec.node = builder->GetNode();

         m_Items.push_back(std::pair<HudItemPtr, TextSpec>(item, text_spec));

         m_Root->AddChild(text_spec.node);

         return m_Items.size() - 1;
      }
   }

   return ion::base::kInvalidIndex;
}

bool Hud::Update(double elapsedTimeInSec, double secSinceLastFrame)
{
   //Get the width and height from the uniforms
   const VectorBase2i& viewSize = m_ViewportUniforms->GetUniforms()[m_ViewportUniforms->GetUniformIndex(UNIFORM_GLOBAL_VIEWPORTSIZE)].GetValue<VectorBase2i>();

   //Set the enabled flag for all nodes
   bool processingActive = m_ProgressItem.first->IsProcessingActive();

   m_ProgressItem.second.node->Enable(processingActive);

   for (size_t i = 0; i < m_Items.size(); ++i)
   {
      m_Items[i].second.node->Enable(!processingActive);
   }

   if (m_ProgressItem.first->IsProcessingActive())
   {
      TextSpec& spec = m_ProgressItem.second;
      LayoutOptions& region = spec.region;

      int dotCount = (int)ceil(fmod(elapsedTimeInSec, 3.0));

      spec.text = m_ProgressItem.first->GetText();

      Lines lines = ion::base::SplitString(spec.text, "\n");

      //Set the current point
      spec.region.target_point = Point2f(0.5f, 0.5f);

      FreeTypeFont* ftFont = static_cast<FreeTypeFont *>(spec.builder->GetFont().Get());

      // Determine the size of the text.
      TextSize text_size = ComputeTextSize(*ftFont, spec.region, lines);

      spec.text = m_ProgressItem.first->GetText() + std::string(dotCount, '.');
      lines = ion::base::SplitString(spec.text, "\n");

      TextSize text_size_with_dots = ComputeTextSize(*ftFont, spec.region, lines);

      auto sizeWithOutDots = Vector2f(text_size.rect_size_in_pixels[0] / viewSize[0], text_size.rect_size_in_pixels[1] / viewSize[1]);

      spec.region.target_size = Vector2f(text_size_with_dots.rect_size_in_pixels[0] / viewSize[0], text_size_with_dots.rect_size_in_pixels[1] / viewSize[1]);

      spec.region.target_point[0] += (spec.region.target_size[0] - sizeWithOutDots[0]) / 2.0f;

      spec.builder->Build(spec.builder->GetFont()->BuildLayout(spec.text, spec.region), ion::gfx::BufferObject::kStreamDraw);

      return false;
   }

   //Start 5 pixels off the border
   Point2f currPoint = Point2f(5.0f / static_cast<float>(viewSize[0]), 1.0f - (5.0f / static_cast<float>(viewSize[1])));

   float resetX = currPoint[0];

   // Update the sizes in all fixed-size regions.
   const size_t numItems = m_Items.size();
   for (size_t i = 0; i < numItems; ++i)
   {
      TextSpec& spec = m_Items[i].second;
      LayoutOptions& region = spec.region;

      spec.text = m_Items[i].first->GetText();

      const Lines lines = ion::base::SplitString(spec.text, "\n");

      //Set the current point
      spec.region.target_point = Point2f(resetX, currPoint[1]);

      FreeTypeFont* ftFont = static_cast<FreeTypeFont *>(spec.builder->GetFont().Get());

      // Determine the size of the text.
      TextSize text_size = ComputeTextSize(*ftFont, spec.region, lines);

      spec.region.target_size = Vector2f(text_size.rect_size_in_pixels[0] / viewSize[0], text_size.rect_size_in_pixels[1] / viewSize[1]);
      spec.region.target_point[1] -= spec.region.target_size[1];

      spec.builder->Build(spec.builder->GetFont()->BuildLayout(spec.text, spec.region), ion::gfx::BufferObject::kStreamDraw);

      currPoint = spec.region.target_point;
   }

   return true;
}
