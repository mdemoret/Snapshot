// Snapshot.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ion/portgfx/glheaders.h"
#include <GLFW/glfw3.h>
#include "Window.hpp"
#include <vector>
#include "FinalAction.hpp"
#include "ion/base/staticsafedeclare.h"

using namespace Snapshot::Util;

ion::port::Mutex s_WindowMutex;


static void exit_callback();
static void error_callback(int error, const char* description);
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void character_callback(GLFWwindow* window, unsigned int codepoint);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void drop_callback(GLFWwindow* window, int count, const char** paths);
static void window_refresh_callback(GLFWwindow* window);

static Snapshot::Window * _Window = nullptr;

void SetCallbacks(Snapshot::Window * window)
{
   GLFWwindow * windowPtr = window->GetGlfwPtr();

   //Set all of the callbacks for the window
   glfwSetFramebufferSizeCallback(windowPtr, framebuffer_size_callback);

   glfwSetKeyCallback(windowPtr, key_callback);
   glfwSetCharCallback(windowPtr, character_callback);
   glfwSetCursorPosCallback(windowPtr, cursor_position_callback);
   glfwSetMouseButtonCallback(windowPtr, mouse_button_callback);
   glfwSetScrollCallback(windowPtr, scroll_callback);
   glfwSetDropCallback(windowPtr, drop_callback);
   glfwSetWindowRefreshCallback(windowPtr, window_refresh_callback);
}

int main()
{
   std::atexit(exit_callback);

   glfwSetErrorCallback(error_callback);

   if (!glfwInit())
      exit(EXIT_FAILURE);

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

   try
   {
      finally windowDeleter([&]() {

         //Make sure the window is destroyed first
         ion::base::LockGuard guard(&s_WindowMutex);

         //Clean up the window resources
         if (_Window)
         {
            delete _Window;
            _Window = nullptr;
         }
      });

      //Create the window, this initializes the necessary glfw resources
      _Window = new Snapshot::Window(640, 480, "Snapshot Tool");

      SetCallbacks(_Window);
      
      glfwSwapInterval(1); //Indicates that SwapBuffers should be called no more than once per frame
      glfwWaitEventsTimeout(1.0); //Specifies that we will update once per second to keep things alive but not burn CPU

      //Main Loop
      while (!glfwWindowShouldClose(_Window->GetGlfwPtr()))
      {
         //First update to get the windows synced with any events
         _Window->Update();

         //Render the window, this calls SwapBuffers on its own
         _Window->Render();

         glfwSwapBuffers(_Window->GetGlfwPtr());
         glfwWaitEvents();
      }

   }
   catch(std::exception)
   {
      exit(EXIT_FAILURE);
   }

   exit(EXIT_SUCCESS);
}

void exit_callback()
{
   //Make sure the window is destroyed first
   ion::base::LockGuard guard(&s_WindowMutex);

   glfwTerminate();
}

void error_callback(int error, const char* description)
{
   puts(description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
   _Window->ProcessResize(ion::math::Vector2ui(width, height));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   _Window->ProcessKey(key, scancode, action, mods);
}

void character_callback(GLFWwindow* window, unsigned int codepoint)
{}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
   _Window->ProcessMouseMove(ion::math::Point2d(xpos, ypos));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
   _Window->ProcessMouseButton(button, action, mods);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
   _Window->ProcessScroll(ion::math::Vector2d(xoffset, yoffset));
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
   std::vector<std::string> files(count);

   for (int i = 0; i < count; i++)
   {
      files[i] = std::string(paths[i]);
   }

   _Window->ProcessFileDrop(files);
}

void window_refresh_callback(GLFWwindow* window)
{
   _Window->Render();
}