#include "CollisionShape.h"
#include <Physics/BulletConv.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

namespace Physics
{

CCollisionShape::CCollisionShape(btCollisionShape* pBtShape, const vector3& Offset, const vector3& Scaling)
	: _pBtShape(pBtShape)
	, _Offset(Offset.x * Scaling.x, Offset.y * Scaling.y, Offset.z * Scaling.z)
{
	// NB: it is very important to store resource object pointer inside a bullet shape.
	// This pointer is used in manual refcounting in CMovableCollider and similar shape users.
	n_assert(_pBtShape && !_pBtShape->getUserPointer());
	_pBtShape->setUserPointer(this);
	_pBtShape->setLocalScaling(VectorToBtVector(Scaling));
}
//---------------------------------------------------------------------

CCollisionShape::~CCollisionShape()
{
	delete _pBtShape;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CloneWithScaling(const vector3& Scaling) const
{
	const auto& CurrScaling = _pBtShape->getLocalScaling();
	const vector3 UnscaledOffset(_Offset.x / CurrScaling.x(), _Offset.y / CurrScaling.y(), _Offset.z / CurrScaling.z());

	switch (_pBtShape->getShapeType())
	{
		case BOX_SHAPE_PROXYTYPE:
		{
			const btVector3 UnscaledHalfExtents = static_cast<btBoxShape*>(_pBtShape)->getHalfExtentsWithMargin() / CurrScaling;
			return n_new(CCollisionShape(new btBoxShape(UnscaledHalfExtents), UnscaledOffset, Scaling));
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateSphere(float Radius, const vector3& Offset, const vector3& Scaling)
{
	if (Radius <= 0.f) return nullptr;

	return n_new(CCollisionShape(new btSphereShape(Radius), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateBox(const vector3& Size, const vector3& Offset, const vector3& Scaling)
{
	if (Size.x <= 0.f || Size.y <= 0.f || Size.z <= 0.f) return nullptr;

	return n_new(CCollisionShape(new btBoxShape(VectorToBtVector(Size * 0.5f)), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleX(float Radius, float CylinderLength, const vector3& Offset, const vector3& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShapeX(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleY(float Radius, float CylinderLength, const vector3& Offset, const vector3& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShape(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleZ(float Radius, float CylinderLength, const vector3& Offset, const vector3& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShapeZ(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

}
