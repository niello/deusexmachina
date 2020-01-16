#pragma once
#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>
#include <Render/Light.h>

// Light attribute is a scene node attribute that stores a light source inside

class CAABB;

namespace Scene
{
	class CSPS;
	struct CSPSRecord;
}

namespace Frame
{

class CLightAttribute: public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	Render::CLight     Light;
	Scene::CSPS*       pSPS = nullptr;
	Scene::CSPSRecord* pSPSRecord = nullptr;
	U32                LastTransformVersion = 0;

	virtual void                  OnActivityChanged(bool Active) override;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual Scene::PNodeAttribute Clone();
	void                          UpdateInSPS(Scene::CSPS& SPS);

	void                          CalcFrustum(matrix44& OutFrustum) const;
	bool                          GetGlobalAABB(CAABB& OutBox) const;
	const vector3&                GetPosition() const { return _pNode->GetWorldMatrix().Translation(); }
	vector3                       GetDirection() const { return -_pNode->GetWorldMatrix().AxisZ(); }
	const vector3&                GetReverseDirection() const { return _pNode->GetWorldMatrix().AxisZ(); }
	const Render::CLight&         GetLight() const { return Light; }
};

typedef Ptr<CLightAttribute> PLightAttribute;

}
