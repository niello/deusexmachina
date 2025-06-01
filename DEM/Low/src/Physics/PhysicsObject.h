#pragma once
#include <Core/Object.h>
#include <Physics/PhysicsMaterial.h>
#include <Data/StringID.h>
#include <Math/SIMDMath.h>
#include <any>

// Base class for all physics objects. Has collision shape and transformation.

class btCollisionObject;
class btTransform;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
class CCollisionShape;

class CPhysicsObject: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(Physics::CPhysicsObject, DEM::Core::CObject);

protected:

	btCollisionObject* _pBtObject = nullptr;
	PPhysicsLevel      _Level;
	CStrID             _CollisionGroupID;
	CStrID             _CollisionMaskID;
	std::any           _UserData;
	bool               _IsShapeShared = true; // Shape may be shared with other objects by default. Private copy is created on demand.

	virtual void           AttachToLevelInternal() = 0;
	virtual void           RemoveFromLevelInternal() = 0;

	void                   ConstructInternal(btCollisionObject* pBtObject, const CPhysicsMaterial& Material);
	bool                   UnshareShapeIfNecessary(const rtm::vector4f& NewScaling);
	bool                   PrepareTransform(const rtm::matrix3x4f& NewTfm, btTransform& OutTfm);

public:

	CPhysicsObject(CStrID CollisionGroupID, CStrID CollisionMaskID);
	virtual ~CPhysicsObject() override;

	void                   AttachToLevel(CPhysicsLevel& Level);
	void                   RemoveFromLevel();

	virtual void           SetTransform(const rtm::matrix3x4f& Tfm) = 0;
	virtual void           GetTransform(rtm::matrix3x4f& OutTfm) const = 0;
	virtual void           GetGlobalAABB(Math::CAABB& OutBox) const = 0;
	void                   GetPhysicsAABB(Math::CAABB& OutBox) const;
	const CCollisionShape* GetCollisionShape() const;
	CStrID                 GetCollisionGroup() const { return _CollisionGroupID; }
	CStrID                 GetCollisionMask() const { return _CollisionMaskID; }
	virtual void           SetActive(bool Active, bool Always = false) = 0;
	bool                   IsActive() const;
	bool                   IsAlwaysActive() const;
	CPhysicsLevel*         GetLevel() const { return _Level.Get(); }
	const std::any&        UserData() const { return _UserData; }
	std::any&              UserData() { return _UserData; }
	btCollisionObject*     GetBtObject() const { return _pBtObject; }
};

typedef Ptr<CPhysicsObject> PPhysicsObject;

}
