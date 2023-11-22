#pragma once
#include <Scene/NodeAttribute.h>
#include <Frame/GraphicsScene.h>
#include <Render/RenderFwd.h>

// Light attribute is a scene node attribute that contains light source params.
// See subclasses for different light source types.

namespace Math
{
	struct CAABB;
}

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
	float                   _MaxDistance = std::numeric_limits<float>().max(); //!!!TODO: load from settings!
	bool                    _CastsShadow : 1;
	bool                    _DoOcclusionCulling : 1;

	virtual void            OnActivityChanged(bool Active) override;

public:

	CLightAttribute();

	virtual Render::PLight  CreateLight() const = 0;
	virtual void            UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const = 0;
	virtual rtm::vector4f   GetLocalSphere() const = 0;
	virtual bool            GetLocalAABB(Math::CAABB& OutBox) const = 0;
	bool                    GetGlobalAABB(Math::CAABB& OutBox) const;
	void                    UpdateInGraphicsScene(CGraphicsScene& Scene);
	CGraphicsScene::HRecord GetSceneHandle() const { return _SceneRecordHandle; }

	float                   GetMaxDistance() const { return _MaxDistance; }
	virtual bool            IntersectsWith(rtm::vector4f_arg0 Sphere) const = 0;
	virtual U8              TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const = 0;
	virtual bool            DoesEmitAnyEnergy() const = 0;
};

using PLightAttribute = Ptr<CLightAttribute>;

}
