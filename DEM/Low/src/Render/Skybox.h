#pragma once
#include <Render/Renderable.h>
#include <Render/Mesh.h>

// Skybox is a piece of generated geometry that draws backgroung of a scene (typically sky)

namespace Render
{

class CSkybox: public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PMaterial Material;
	PMesh     Mesh;
	U32       ShaderTechIndex = INVALID_INDEX_T<U32>;
};

typedef Ptr<CSkybox> PSkybox;

}
