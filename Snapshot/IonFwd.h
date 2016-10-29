#pragma once

#include "ion/base/referent.h"

namespace ion {namespace base{
template<typename T>
class Setting;
class DataContainer;

template <typename T>
class VectorDataContainer;

typedef base::ReferentPtr<DataContainer>::Type DataContainerPtr;
}

namespace gfx{
class StateTable;
class UniformBlock;
class GraphicsManager;
class Renderer;
class Node;
class BufferObject;

typedef base::ReferentPtr<GraphicsManager>::Type GraphicsManagerPtr;
typedef base::ReferentPtr<Renderer>::Type RendererPtr;
typedef base::ReferentPtr<Node>::Type NodePtr;
typedef base::ReferentPtr<UniformBlock>::Type UniformBlockPtr;
typedef base::ReferentPtr<StateTable>::Type StateTablePtr;
typedef base::ReferentPtr<BufferObject>::Type BufferObjectPtr;
}

namespace gfxutils{
class ShaderManager;
class Frame;

typedef base::ReferentPtr<ShaderManager>::Type ShaderManagerPtr;
typedef base::ReferentPtr<Frame>::Type FramePtr;
}

namespace text{
class FontManager;
class BasicBuilder;
class Font;
class FontImage;

typedef base::ReferentPtr<FontManager>::Type FontManagerPtr;
typedef base::ReferentPtr<BasicBuilder>::Type BasicBuilderPtr;
typedef base::ReferentPtr<Font>::Type FontPtr;
typedef base::ReferentPtr<FontImage>::Type FontImagePtr;

}

namespace remote{
class RemoteServer;
} // namespace remote
} // namespace ion
