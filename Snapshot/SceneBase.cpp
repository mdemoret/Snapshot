#include "SceneBase.hpp"
#include "ion/gfxutils/frame.h"
#include "ion/gfx/graphicsmanager.h"
#include "ion/gfx/renderer.h"
#include "ion/gfxutils/shadermanager.h"
#include "Camera.hpp"
#include "ion/remote/nodegraphhandler.h"
#include "Window.hpp"
#include "ion/remote/calltracehandler.h"
#include "ion/remote/resourcehandler.h"
#include "ion/remote/settinghandler.h"
#include "ion/remote/shaderhandler.h"
#include "ion/remote/tracinghandler.h"
#include "ion/remote/remoteserver.h"
#include "ion/text/fontmanager.h"

using namespace ion::gfx;
using namespace ion::gfxutils;
using namespace ion::math;

namespace Snapshot{

SceneBase::SceneBase():
   m_Frame(new Frame),
   m_GraphicsManager(new GraphicsManager()),
   m_Renderer(new Renderer(m_GraphicsManager)),
   m_ShaderManager(new ShaderManager()),
   m_FontManager(new ion::text::FontManager())
{
   GetGraphicsManager()->EnableErrorChecking(true);

   //First create a root node
   m_Root = NodePtr(new Node());
   m_Root->SetLabel("Root");

   StateTablePtr state_table(new StateTable(800, 600));
   //state_table->SetViewport(
   //   Range2i::BuildWithSize(Point2i(0, 0), Vector2i(width, height)));
   state_table->SetClearColor(Vector4f(1.0f, 1.0f, 1.0f, 1.0f));
   state_table->SetClearDepthValue(1.0f);
   state_table->SetDepthFunction(StateTable::kDepthLessOrEqual);
   state_table->Enable(StateTable::kMultisample, true);
   state_table->Enable(StateTable::kDepthTest, true);
   state_table->Enable(StateTable::kBlend, true);
   state_table->Enable(StateTable::kCullFace, true);
   state_table->SetBlendFunctions(StateTable::kSrcAlpha,
                                  StateTable::kOneMinusSrcAlpha,
                                  StateTable::kOne,
                                  StateTable::kZero);
   m_Root->SetStateTable(state_table);

   //Create the camera, this will create a uniform block to use
   m_Camera = std::make_shared<Camera>(m_Root->GetStateTable());

   //Set the root viewport uniform
   m_Root->AddUniformBlock(m_Camera->GetViewportUniforms());

   InitRemoteHandlers(vector<NodePtr>(1, m_Root));
}

SceneBase::~SceneBase() {}

bool SceneBase::Update(double elapsedTimeInSec, double secSinceLastFrame)
{
   m_Camera->UpdateUniforms();

   return true;
}

void SceneBase::Render()
{
   m_Frame->Begin();
   RenderFrame();
   m_Frame->End();
}

void SceneBase::InitRemoteHandlers(const vector<NodePtr>& nodes_to_track)
{
#ifndef ION_PRODUCTION

   auto remote = Window::ResetRemoteServer();

   remote->RegisterHandler(ion::remote::HttpServer::RequestHandlerPtr(new ion::remote::SettingHandler()));

#ifdef _DEBUG

   ion::remote::NodeGraphHandlerPtr ngh(new ion::remote::NodeGraphHandler);
   ngh->SetFrame(m_Frame);
   for (size_t i = 0; i < nodes_to_track.size(); ++i)
      ngh->AddNode(nodes_to_track[i]);

   remote->RegisterHandler(ngh);
   remote->RegisterHandler(ion::remote::HttpServer::RequestHandlerPtr(new ion::remote::CallTraceHandler()));
   remote->RegisterHandler(ion::remote::HttpServer::RequestHandlerPtr(new ion::remote::ResourceHandler(GetRenderer())));
   remote->RegisterHandler(ion::remote::HttpServer::RequestHandlerPtr(new ion::remote::ShaderHandler(GetShaderManager(), GetRenderer())));
   remote->RegisterHandler(ion::remote::HttpServer::RequestHandlerPtr(new ion::remote::TracingHandler(GetFrame(), GetRenderer())));
#endif

#endif
}
}
