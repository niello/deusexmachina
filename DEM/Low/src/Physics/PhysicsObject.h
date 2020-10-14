#pragma once
#include <Core/Object.h>
#include <Physics/PhysicsMaterial.h>
#include <Data/StringID.h>
#include <Math/AABB.h>
#include <any>

// Base class for all physics objects. Has collision shape and transformation.

class btCollisionObject;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
class CCollisionShape;

class CPhysicsObject: public Core::CObject
{
	RTTI_CLASS_DECL(Physics::CPhysicsObject, Core::CObject);

protected:

	btCollisionObject* _pBtObject = nullptr;
	PPhysicsLevel      _Level;
	CStrID             _CollisionGroupID;
	CStrID             _CollisionMaskID;
	std::any           _UserData;

	virtual void           AttachToLevelInternal() = 0;
	virtual void           RemoveFromLevelInternal() = 0;

	void                   SetupInternalObject(btCollisionObject* pBtObject, const CCollisionShape& Shape, const CPhysicsMaterial& Material);

public:

	CPhysicsObject(CStrID CollisionGroupID, CStrID CollisionMaskID);

	void                   AttachToLevel(CPhysicsLevel& Level);
	void                   RemoveFromLevel();

	virtual void           SetTransform(const matrix44& Tfm) = 0;
	virtual void           GetTransform(matrix44& OutTfm) const = 0;
	virtual void           GetGlobalAABB(CAABB& OutBox) const = 0;
	void                   GetPhysicsAABB(CAABB& OutBox) const;
	const CCollisionShape* GetCollisionShape() const;
	CStrID                 GetCollisionGroup() const { return _CollisionGroupID; }
	CStrID                 GetCollisionMask() const { return _CollisionMaskID; }
	virtual void           SetActive(bool Active, bool Always = false) = 0;
	bool                   IsActive() const;
	bool                   IsAlwaysActive() const;
	CPhysicsLevel*         GetLevel() const { return _Level.Get(); }
	const std::any&        UserData() const { return _UserData; }
	std::any&              UserData() { return _UserData; }
};

typedef Ptr<CPhysicsObject> PPhysicsObject;

}
