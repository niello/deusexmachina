#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/AABB.h>

// Scene node attribute, that adds a link from scene node transformation
// to a movable collision object in a physics world. Use this attribute to
// add a collision shape to an object in a scene.

//???!!!can also add CNodeAttrJoint! to connect rigid bodies to non-physical scene nodes

namespace Physics
{
typedef Ptr<class CPhysicsObject> PPhysicsObject;
class CPhysicsLevel;

class CCollisionAttribute: public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	PPhysicsObject Collider;
	U32            LastTransformVersion = 0;
	CStrID         ShapeUID;
	CStrID         CollisionGroupID;
	CStrID         CollisionMaskID;
	bool           Static = false;

	virtual void                  OnActivityChanged(bool Active) override;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual void                  UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount) override;
	bool                          ValidateResources(Resources::CResourceManager& ResMgr, CPhysicsLevel& Level);
	bool                          GetGlobalAABB(CAABB& OutBox) const;
};

typedef Ptr<CCollisionAttribute> PCollisionAttribute;

}
