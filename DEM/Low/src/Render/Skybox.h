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

	virtual PRenderable Clone() override;
	virtual bool        GetLocalAABB(CAABB& OutBox, UPTR LOD) const override { OutBox = CAABB::Empty; OK; }	// Infinite size
};

typedef Ptr<CSkybox> PSkybox;

}
