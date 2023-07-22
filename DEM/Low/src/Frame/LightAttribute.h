#pragma once
#include <Scene/NodeAttribute.h>
#include <Frame/GraphicsScene.h>
#include <Render/RenderFwd.h>

// Light attribute is a scene node attribute that contains light source params.
// See subclasses for different light source types.

class CAABB;

namespace Frame
{
class CGraphicsResourceManager;
class CGraphicsScene;

class CLightAttribute: public Scene::CNodeAttribute
{
	RTTI_CLASS_DECL(Frame::CLightAttribute, Scene::CNodeAttribute);

protected:

	CGraphicsScene*         _pScene = nullptr;
	CGraphicsScene::HRecord _SceneRecordHandle = {};
	U32                     _LastTransformVersion = 0;
	float                   _MaxDistanceSq = std::numeric_limits<float>().max(); //!!!TODO: load from settings!
	bool                    _CastsShadow : 1;
	bool                    _DoOcclusionCulling : 1;

	virtual void           OnActivityChanged(bool Active) override;

public:

	CLightAttribute();

	virtual Render::PLight  CreateLight() const = 0;
	virtual void            UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const = 0;
	virtual bool            GetLocalAABB(CAABB& OutBox) const = 0;
	bool                    GetGlobalAABB(CAABB& OutBox) const;
	void                    UpdateInGraphicsScene(CGraphicsScene& Scene);
	CGraphicsScene::HRecord GetSceneHandle() const { return _SceneRecordHandle; }

	float                  GetMaxDistanceSquared() const { return _MaxDistanceSq; }
	virtual bool           DoesEmitAnyEnergy() const = 0;

	// get intensity at point
	// calc intersection / influence for AABB or OBB - get max intensity for box (= get intensity at closest point)
};

using PLightAttribute = Ptr<CLightAttribute>;

}
