#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/AABB.h>

// Scene node attribute, that adds a link from scene node transformation
// to a movable collision object in a physics world. Use this attribute to
// add a collision shape to an object in a scene.

//???!!!can also add CNodeAttrJoint! to connect rigid bodies to non-physical scene nodes

namespace Physics
{
typedef Ptr<class CMovableCollider> PMovableCollider;
class CPhysicsLevel;

class CCollisionAttribute: public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID           ShapeUID;
	CStrID           CollisionGroupID;
	CStrID           CollisionMaskID;
	PMovableCollider Collider;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual void                  Update(const vector3* pCOIArray, UPTR COICount) override;
	bool                          ValidateResources(Resources::CResourceManager& ResMgr, CPhysicsLevel& Level);
	bool                          GetGlobalAABB(CAABB& OutBox) const;
};

typedef Ptr<CCollisionAttribute> PCollisionAttribute;

}
