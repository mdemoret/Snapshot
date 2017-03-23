#include "KeyboardHandler.hpp"
#include "GLFW/glfw3.h"
#include "ion/base/settingmanager.h"
#include "Macros.h"
#include "FileManager.hpp"
#include "Camera.hpp"

namespace Snapshot{
KeyboardHandler::KeyboardHandler() {}

KeyboardHandler::~KeyboardHandler() {}

void KeyboardHandler::Escape(int action)
{
   if (action == GLFW_PRESS)
      m_FileManager->Cancel();
}

void KeyboardHandler::LeftArrow(int action)
{
   if (action == GLFW_RELEASE)
      return;

   //Try to increment the epoch index if possible
   auto setting = dynamic_cast<ion::base::Setting<uint32_t>*>(ion::base::SettingManager::GetSetting(SETTINGS_INPUT_EPOCH_INDEX));

   if (setting != nullptr && setting->GetValue() > 0)
      setting->SetValue(setting->GetValue() - 1);
}

void KeyboardHandler::RightArrow(int action)
{
   if (action == GLFW_RELEASE)
      return;

   //Try to increment the epoch index if possible
   auto setting = dynamic_cast<ion::base::Setting<uint32_t>*>(ion::base::SettingManager::GetSetting(SETTINGS_INPUT_EPOCH_INDEX));

   if (setting != nullptr && setting->GetValue() < m_FileManager->GetEpochs().size() - 1)
      setting->SetValue(setting->GetValue() + 1);
}

void KeyboardHandler::R(int action, int modifier) 
{
   if (action != GLFW_PRESS)
      return;

   //If shift is held, then reset to Conjunction plane
   if (modifier == GLFW_MOD_SHIFT)
   {

   }
   else
   {
      m_Camera->ResetView();
   }
}

void KeyboardHandler::Initialize(const std::shared_ptr<FileManager>& fileManager, const std::shared_ptr<Camera>& camera)
{
   m_FileManager = fileManager;
   m_Camera = camera;
}
}
