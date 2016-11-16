#pragma once

#include "IonFwd.h"

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

   void SetFileManager(const std::shared_ptr<FileManager> & fileManager);

private:
   std::shared_ptr<FileManager> m_FileManager;
};
}
