#pragma once

#include "ion/base/referent.h"
#include "IonFwd.h"
#include <memory>
#include "ion/text/layout.h"

class ProgressHudItem;

class HudItem
{
public:
   virtual ~HudItem() {}
   explicit HudItem(const std::string & startString);

   void SetText(const std::string & text);
   virtual std::string GetText() const;

private:
   std::string m_Text;
};
typedef std::shared_ptr<HudItem> HudItemPtr;

class ProgressHandler
{
public:
   ~ProgressHandler();

   void SetProgressFunc(const std::function<std::string()> & getProgress);

private:
   friend class ProgressHudItem;

   explicit ProgressHandler(ProgressHudItem * parent);

   static std::string DefaultHandler();

   ProgressHudItem * m_Parent;

   std::function<std::string()> m_GetProgressFunc;
};

class ProgressHudItem : public HudItem
{
public:
   explicit ProgressHudItem(const std::string& startString)
      : HudItem(startString) {}

   virtual ~ProgressHudItem() override;

   ProgressHandler GetProgressHandler();

   bool IsProcessingActive() const;

   virtual std::string GetText() const override;

private:
   friend class ProgressHandler;

   std::function<std::string()> m_GetProgressFunc;
};
typedef std::shared_ptr<ProgressHudItem> ProgressHudItemPtr;

// This class implements a very simple HUD (heads-up display) for Ion demos.
// Right now it provides just a simple frames-per-second display.
class Hud
{
public:
   Hud(const ion::text::FontManagerPtr& font_manager, const ion::gfxutils::ShaderManagerPtr& shader_manager, const ion::gfx::UniformBlockPtr& viewportUniforms);
   virtual ~Hud();

   ProgressHudItemPtr GetProgressHudItem() const;

   // Initializes the display of frames-per-second text.
   size_t AddHudItem(const HudItemPtr& item);

   // This must be called every frame for the fps display to work. Returns whether or not to continue processing update
   // This can happen if something is loading
   bool Update(double elapsedTimeInSec, double secSinceLastFrame);

   // Returns the root node for the HUD graph. Render this node to show the HUD.
   const ion::gfx::NodePtr& GetRootNode() const { return m_Root; }

private:
   // Stores information needed to process each text string.
   struct TextSpec
   {
      ion::text::LayoutOptions region;
      std::string text;
      ion::text::BasicBuilderPtr builder;
      ion::gfx::NodePtr node;
   };

   // Initializes a font, returning a pointer to a Font. Logs a message and
   // returns a NULL pointer on error.
   ion::text::FontPtr InitFont(const std::string& font_name, size_t size_in_pixels, size_t sdf_padding) const;

   // Initializes and returns a StaticFontImage that uses the given Font and
   // GlyphSet. It caches it in the FontManager so that subsequent calls
   // with the same key use the same instance.
   const ion::text::FontImagePtr InitFontImage(const std::string& key, const ion::text::FontPtr& font) const;

   // Adds a text string. The returned ID is used to identify the text in
   // subsequent calls to the TextHelper. Returns kInvalidIndex on error.
   size_t AddText(const ion::text::FontImagePtr& font_image,
                  const ion::text::LayoutOptions& region, const std::string& text);


   // Root node of the HUD graph.
   ion::gfx::NodePtr m_Root;

   // FontManager used for initializing fonts.
   ion::text::FontManagerPtr m_FontManager;

   // ShaderManager used for composing text shaders.
   ion::gfxutils::ShaderManagerPtr m_ShaderManager;

   // Width and height of the HUD, which are used to manage fixed-size regions.
   ion::gfx::UniformBlockPtr m_ViewportUniforms;

   std::pair<ProgressHudItemPtr, TextSpec> m_ProgressItem;

   // Data for each text string added.
   std::vector<std::pair<HudItemPtr, TextSpec>> m_Items;
};
