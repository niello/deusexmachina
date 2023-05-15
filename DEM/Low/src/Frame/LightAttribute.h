#pragma once
#include <Scene/NodeAttribute.h>
#include <Frame/GraphicsScene.h>

// Light attribute is a scene node attribute that contains light source params.
// See subclasses for different light source types.

class CAABB;

namespace Render
{
	typedef std::unique_ptr<class CLight> PLight;
}

namespace Frame
{
class CGraphicsResourceManager;
class CGraphicsScene;

class CLightAttribute: public Scene::CNodeAttribute
{
	RTTI_CLASS_DECL(Frame::CLightAttribute, Scene::CNodeAttribute);

protected:

	CGraphicsScene*         pScene = nullptr;
	CGraphicsScene::HRecord SceneRecordHandle = {};
	U32                     LastTransformVersion = 0;
	bool                    _CastsShadow : 1;
	bool                    _DoOcclusionCulling : 1;

	virtual void           OnActivityChanged(bool Active) override;

public:

	CLightAttribute();

	virtual Render::PLight CreateLight(CGraphicsResourceManager& ResMgr) const = 0;
	virtual bool           GetLocalAABB(CAABB& OutBox) const = 0;
	bool                   GetGlobalAABB(CAABB& OutBox) const;
	void                   UpdateInGraphicsScene(CGraphicsScene& Scene);

	//???in attr or GPU light?
	// get intensity at point
	// calc intersection / influence for AABB or OBB - get max intensity for box (= get intensity at closest point)
};

typedef Ptr<CLightAttribute> PLightAttribute;

}
