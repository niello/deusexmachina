#pragma once
#include <Render/Renderable.h>
#include <Render/Mesh.h>

// Skybox is a piece of generated geometry that draws backgroung of a scene (typically sky)

namespace Resources
{
	typedef Ptr<class CResource> PResource;
}

namespace Render
{

class CSkybox: public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PMaterial				Material;
	PMesh					Mesh;
};

typedef Ptr<CSkybox> PSkybox;

}
