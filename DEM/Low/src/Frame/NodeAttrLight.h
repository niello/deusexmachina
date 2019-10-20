#pragma once
#ifndef __DEM_L1_FRAME_NODE_ATTR_LIGHT_H__
#define __DEM_L1_FRAME_NODE_ATTR_LIGHT_H__

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

namespace Render
{
	class CLight;
}

namespace Frame
{

class CNodeAttrLight: public Scene::CNodeAttribute
{
	__DeclareClass(CNodeAttrLight);

protected:

	Render::CLight		Light;
	Scene::CSPS*		pSPS;
	Scene::CSPSRecord*	pSPSRecord;		// nullptr if oversized

	virtual void	OnDetachFromScene();

public:

	CNodeAttrLight(): pSPS(nullptr), pSPSRecord(nullptr) {}

	virtual bool					LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual Scene::PNodeAttribute	Clone();
	void							UpdateInSPS(Scene::CSPS& SPS);

	void							CalcFrustum(matrix44& OutFrustum) const;
	bool							GetGlobalAABB(CAABB& OutBox) const;
	const vector3&					GetPosition() const { return pNode->GetWorldMatrix().Translation(); }
	vector3							GetDirection() const { return -pNode->GetWorldMatrix().AxisZ(); }
	const vector3&					GetReverseDirection() const { return pNode->GetWorldMatrix().AxisZ(); }
	const Render::CLight&			GetLight() const { return Light; }
};

typedef Ptr<CNodeAttrLight> PNodeAttrLight;

}

#endif
