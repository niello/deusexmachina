#include "CollisionShape.h"
#include <Physics/BulletConv.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{
RTTI_CLASS_IMPL(Physics::CCollisionShape, Resources::CResourceObject);

CCollisionShape::CCollisionShape(btCollisionShape* pShape)
	: pBtShape(pShape)
{
	// NB: it is very important to store resource object pointer inside a bullet shape.
	// This pointer is used in manual refcounting in CMovableCollider and similar shape users.
	n_assert(pBtShape && !pBtShape->getUserPointer());
	pBtShape->setUserPointer(this);
}
//---------------------------------------------------------------------

CCollisionShape::~CCollisionShape()
{
	delete pBtShape;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateSphere(const vector3& Offset, float Radius)
{
	if (Radius <= 0.f) return nullptr;

	return n_new(Physics::CCollisionShape(new btSphereShape(Radius)));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateBox(const vector3& Offset, const vector3& Size)
{
	if (Size.x <= 0.f || Size.y <= 0.f || Size.z <= 0.f) return nullptr;

	return n_new(Physics::CCollisionShape(new btBoxShape(VectorToBtVector(Size * 0.5f))));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleY(const vector3& Offset, float Radius, float CylinderLength)
{
	if (CylinderLength <= 0.f) return CreateSphere(Offset, Radius);

	return n_new(Physics::CCollisionShape(new btCapsuleShape(Radius, CylinderLength)));
}
//---------------------------------------------------------------------

}
