#pragma once
#include <Scene/NodeAttribute.h>

// Base attribute class for any renderable scene objects. Initialization is done in two
// phases. First all plain data is initialized on loading. After that the attribute can
// be used as an instance or as a template (create instances with Clone()). Second phase
// is resource validation. It happens only for instances and initializes GPU-dependent
// resources of the attribute, making it renderable on certain GPU.

class CAABB;

namespace Scene
{
	class CSPS;
	struct CSPSRecord;
}

namespace Render
{
	typedef std::unique_ptr<class IRenderable> PRenderable;
}

namespace Frame
{
class CFrameResourceManager;

class CNodeAttrRenderable: public Scene::CNodeAttribute
{
	__DeclareClassNoFactory;

protected:

	Render::PRenderable	Renderable;
	Scene::CSPS*		pSPS = nullptr;
	Scene::CSPSRecord*	pSPSRecord = nullptr;

	virtual void         OnDetachFromScene();

public:

	virtual bool         ValidateResources(CFrameResourceManager& ResMgr) = 0;
	void                 UpdateInSPS(Scene::CSPS& SPS);

	bool                 GetGlobalAABB(CAABB& OutBox, UPTR LOD = 0) const; //!!!can get from a spatial record!
	Render::IRenderable* GetRenderable() const { return Renderable.get(); }
};

typedef Ptr<CNodeAttrRenderable> PNodeAttrRenderable;

}
