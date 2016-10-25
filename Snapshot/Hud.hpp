/**
Copyright 2016 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#pragma once

#include "ion/base/referent.h"
#include "IonFwd.h"
#include <memory>
#include "ion/text/layout.h"

class HudItem
{
public:
   HudItem(const std::string & startString);

   std::string GetText();

private:
   std::string m_Text;
};
typedef std::shared_ptr<HudItem> HudItemPtr;

// This class implements a very simple HUD (heads-up display) for Ion demos.
// Right now it provides just a simple frames-per-second display.
class Hud
{
public:

   Hud(const ion::text::FontManagerPtr& font_manager, const ion::gfxutils::ShaderManagerPtr& shader_manager, const ion::gfx::UniformBlockPtr& viewportUniforms);
   virtual ~Hud();

   // Initializes the display of frames-per-second text.
   size_t AddHudItem(const HudItemPtr& item);

   // This must be called every frame for the fps display to work.
   void Update();

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

   // Data for each text string added.
   std::vector<std::pair<HudItemPtr, TextSpec>> m_Items;
};
