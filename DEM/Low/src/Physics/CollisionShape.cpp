#include "CollisionShape.h"
#include <Physics/BulletConv.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{

CCollisionShape::CCollisionShape(btCollisionShape* pBtShape, const vector3& Offset)
	: _pBtShape(pBtShape)
	, _Offset(Offset)
{
	// NB: it is very important to store resource object pointer inside a bullet shape.
	// This pointer is used in manual refcounting in CMovableCollider and similar shape users.
	n_assert(_pBtShape && !_pBtShape->getUserPointer());
	_pBtShape->setUserPointer(this);
}
//---------------------------------------------------------------------

CCollisionShape::~CCollisionShape()
{
	delete _pBtShape;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateSphere(float Radius, const vector3& Offset)
{
	if (Radius <= 0.f) return nullptr;

	return n_new(Physics::CCollisionShape(new btSphereShape(Radius), Offset));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateBox(const vector3& Size, const vector3& Offset)
{
	if (Size.x <= 0.f || Size.y <= 0.f || Size.z <= 0.f) return nullptr;

	return n_new(Physics::CCollisionShape(new btBoxShape(VectorToBtVector(Size * 0.5f)), Offset));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleY(float Radius, float CylinderLength, const vector3& Offset)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(Physics::CCollisionShape(new btCapsuleShape(Radius, CylinderLength), Offset));
}
//---------------------------------------------------------------------

}
