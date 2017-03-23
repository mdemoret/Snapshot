#pragma once

#include "IonFwd.h"

class Camera;

namespace Snapshot{
class FileManager;

class KeyboardHandler
{
public:
   KeyboardHandler();
   ~KeyboardHandler();

   void Escape(int action);
   void LeftArrow(int action);
   void RightArrow(int action);
   void R(int action, int modifier);

   void Initialize(const std::shared_ptr<FileManager> & fileManager, const std::shared_ptr<Camera> & camera);

private:
   std::shared_ptr<FileManager> m_FileManager;
   std::shared_ptr<Camera> m_Camera;
};
}
