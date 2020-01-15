#pragma once
#include <Core/Object.h>
#include <Math/AABB.h>

// Base class for all physics objects. Has collision shape and transformation.

class btCollisionObject;

namespace Physics
{
typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
class CCollisionShape;

class CPhysicsObject: public Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	btCollisionObject* _pBtObject = nullptr;
	PPhysicsLevel      _Level;

public:

	CPhysicsObject(CPhysicsLevel& Level);
	virtual ~CPhysicsObject() override;

	virtual void           SetTransform(const matrix44& Tfm);
	virtual void           GetTransform(vector3& OutPos, quaternion& OutRot) const;
	void                   GetGlobalAABB(CAABB& OutBox) const;
	void                   GetPhysicsAABB(CAABB& OutBox) const;
	const CCollisionShape* GetCollisionShape() const;
	U16                    GetCollisionGroup() const;
	U16                    GetCollisionMask() const;
	virtual void           SetActive(bool Active, bool Always = false) = 0;
	bool                   IsActive() const;
};

typedef Ptr<CPhysicsObject> PPhysicsObject;

}
