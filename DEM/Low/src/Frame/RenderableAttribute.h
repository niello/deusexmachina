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
class CGraphicsResourceManager;

class CRenderableAttribute: public Scene::CNodeAttribute
{
	RTTI_CLASS_DECL(Frame::CRenderableAttribute, Scene::CNodeAttribute);

protected:

	Scene::CSPS*		pSPS = nullptr;
	Scene::CSPSRecord*	pSPSRecord = nullptr;
	U32                 LastTransformVersion = 0;

	virtual void OnActivityChanged(bool Active) override;

public:

	virtual Render::PRenderable CreateRenderable(CGraphicsResourceManager& ResMgr) const = 0;
	virtual bool                GetLocalAABB(CAABB& OutBox, UPTR LOD = 0) const = 0;
	bool                        GetGlobalAABB(CAABB& OutBox, UPTR LOD = 0) const;
	void                        UpdateInSPS(Scene::CSPS& SPS);

	virtual void                RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
};

typedef Ptr<CRenderableAttribute> PRenderableAttribute;

}
