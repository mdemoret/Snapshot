#pragma once

#include "IonFwd.h"
#include "ion/math/vector.h"
#include "ion/math/angle.h"

class Camera;

namespace Snapshot{
class SceneBase
{
public:
   SceneBase();
   virtual ~SceneBase();

   virtual void Update();
   void Render();

   const std::shared_ptr<Camera>& GetCamera() const;

protected:
   // Returns the Frame set up by the constructor.
   const ion::gfxutils::FramePtr& GetFrame() const;

   // Returns the GraphicsManager set up by the constructor.
   const ion::gfx::GraphicsManagerPtr& GetGraphicsManager() const;

   // Returns the Renderer set up by the constructor.
   const ion::gfx::RendererPtr& GetRenderer() const;

   // Returns the ShaderManager set up by the constructor.
   const ion::gfxutils::ShaderManagerPtr& GetShaderManager() const;

   const ion::gfx::NodePtr& GetRoot() const;

   virtual void RenderFrame() = 0;


   //---------------------------------------------------------------------------
   // Manager and handler state.

   // Sets up all supported remote handlers. A vector of nodes for the
   // NodeGraphHandler to track must be supplied.
   void InitRemoteHandlers(const std::vector<ion::gfx::NodePtr>& nodes_to_track);

private:
   std::shared_ptr<Camera> m_Camera;

   ion::gfx::NodePtr m_Root;

   //Standard graphics classes
   ion::gfxutils::FramePtr m_Frame;
   ion::gfx::GraphicsManagerPtr m_GraphicsManager;
   ion::gfx::RendererPtr m_Renderer;
   ion::gfxutils::ShaderManagerPtr m_ShaderManager;
};

inline const ion::base::SharedPtr<ion::gfxutils::Frame>& SceneBase::GetFrame() const { return m_Frame; }

inline const ion::base::SharedPtr<ion::gfx::GraphicsManager>& SceneBase::GetGraphicsManager() const { return m_GraphicsManager; }

inline const ion::base::SharedPtr<ion::gfx::Renderer>& SceneBase::GetRenderer() const { return m_Renderer; }

inline const ion::base::SharedPtr<ion::gfxutils::ShaderManager>& SceneBase::GetShaderManager() const { return m_ShaderManager; }

inline const std::shared_ptr<Camera>& SceneBase::GetCamera() const { return m_Camera; }

inline const ion::gfx::NodePtr& SceneBase::GetRoot() const { return m_Root; }
}
