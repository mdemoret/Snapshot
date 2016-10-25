#pragma once
#include <memory>
#include <string>
#include <vector>
#include "IonFwd.h"
#include "ion/math/vector.h"
#include "ion/math/angle.h"

struct GLFWwindow;

namespace Snapshot{
class SceneBase;

class Window
{
public:
   Window(uint32_t width, uint32_t height, const std::string& title);
   virtual ~Window();

   GLFWwindow* GetGlfwPtr() const;

   virtual void Update();
   virtual void Render();

   void ProcessResize(const ion::math::Vector2ui& size);
   void ProcessKey(int key, int scancode, int action, int mods);
   void ProcessMouseMove(const ion::math::Point2d& pos);
   void ProcessMouseButton(int button, int action, int mods);
   void ProcessScroll(const ion::math::Vector2d& offset);
   void ProcessFileDrop(std::vector<std::string>& files);

   static Window* GetWindowFromGlfw(GLFWwindow* windowPtr);

#ifndef ION_PRODUCTION
   static ion::remote::RemoteServer* ResetRemoteServer();

   static ion::remote::RemoteServer* GetRemoteServer();
#endif

protected:
#ifndef ION_PRODUCTION
   static std::unique_ptr<ion::remote::RemoteServer> s_Remote;
#endif


private:
   GLFWwindow* m_WindowPtr;

   SceneBase* m_Scene;

   //Mouse properties
   int m_MouseDownButton;
   ion::math::Point2d m_MouseLastPos;

   ion::math::Angled m_CameraRA;
   ion::math::Angled m_CameraDEC;
   ion::math::Vector1d m_CameraRadius;
};


inline GLFWwindow* Window::GetGlfwPtr() const { return m_WindowPtr; }
}
