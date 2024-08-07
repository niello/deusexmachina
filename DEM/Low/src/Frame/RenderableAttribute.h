#pragma once
#include <Scene/NodeAttribute.h>
#include <Frame/GraphicsScene.h>

// Base attribute class for any renderable scene objects. Initialization is done in two
// phases. First all plain data is initialized on loading. After that the attribute can
// be used as an instance or as a template (create instances with Clone()). Second phase
// is resource validation. It happens only for instances and initializes GPU-dependent
// resources of the attribute, making it renderable on certain GPU.

namespace Render
{
	typedef std::unique_ptr<class IRenderable> PRenderable;
}

namespace Math
{
	struct CAABB;
}

namespace Frame
{
class CView;
class CGraphicsScene;
struct CObjectLightIntersection;

class CRenderableAttribute: public Scene::CNodeAttribute
{
	RTTI_CLASS_DECL(Frame::CRenderableAttribute, Scene::CNodeAttribute);

protected:

	CGraphicsScene*		    _pScene = nullptr;
	CGraphicsScene::HRecord _SceneRecordHandle = {};
	U32                     _LastTransformVersion = 0;

	virtual void OnActivityChanged(bool Active) override;

public:

	enum : U8
	{
		TrackLightContactChanges = 0x01,  // Renderable should track lights making or breaking the contact
		TrackLightRelativeMovement = 0x02 // Renderable should track contacting lights changing their transformation relative to it
	};

	virtual Render::PRenderable CreateRenderable() const = 0;
	virtual void                UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool ViewProjChanged) const = 0;
	virtual void                UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const = 0;
	virtual void                OnLightIntersectionsUpdated() = 0;
	virtual U8                  GetLightTrackingFlags() const = 0;
	virtual bool                GetLocalAABB(Math::CAABB& OutBox, UPTR LOD = 0) const = 0;
	bool                        GetGlobalAABB(Math::CAABB& OutBox, UPTR LOD = 0) const;
	void                        UpdateInGraphicsScene(CGraphicsScene& Scene);
	CGraphicsScene::HRecord     GetSceneHandle() const { return _SceneRecordHandle; }

	virtual void                RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
};

typedef Ptr<CRenderableAttribute> PRenderableAttribute;

}
