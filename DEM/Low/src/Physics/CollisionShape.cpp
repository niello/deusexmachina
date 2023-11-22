#include "CollisionShape.h"
#include <Physics/BulletConv.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>

namespace Physics
{

CCollisionShape::CCollisionShape(btCollisionShape* pBtShape, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
	: _pBtShape(pBtShape)
	, _Offset(rtm::vector_mul(Offset, Scaling))
{
	// NB: it is very important to store resource object pointer inside a bullet shape.
	// This pointer is used in manual refcounting in CMovableCollider and similar shape users.
	n_assert(_pBtShape && !_pBtShape->getUserPointer());
	_pBtShape->setUserPointer(this);
	_pBtShape->setLocalScaling(Math::ToBullet3(Scaling));
}
//---------------------------------------------------------------------

CCollisionShape::~CCollisionShape()
{
	delete _pBtShape;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CloneWithScaling(const rtm::vector4f& Scaling) const
{
	const rtm::vector4f UnscaledOffset = rtm::vector_div(_Offset, Math::FromBullet(_pBtShape->getLocalScaling()));

	switch (_pBtShape->getShapeType())
	{
		case BOX_SHAPE_PROXYTYPE:
		{
			const btVector3 UnscaledHalfExtents = static_cast<btBoxShape*>(_pBtShape)->getHalfExtentsWithMargin() / _pBtShape->getLocalScaling();
			return new CCollisionShape(new btBoxShape(UnscaledHalfExtents), UnscaledOffset, Scaling);
		}
	}

	return nullptr;
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateSphere(float Radius, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (Radius <= 0.f) return nullptr;

	return n_new(CCollisionShape(new btSphereShape(Radius), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateBox(const rtm::vector4f& Size, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (rtm::vector_any_less_equal3(Size, rtm::vector_zero())) return nullptr;

	return new CCollisionShape(new btBoxShape(Math::ToBullet3(rtm::vector_mul(Size, 0.5f))), Offset, Scaling);
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleX(float Radius, float CylinderLength, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShapeX(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleY(float Radius, float CylinderLength, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShape(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateCapsuleZ(float Radius, float CylinderLength, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (CylinderLength <= 0.f) return CreateSphere(Radius, Offset);

	return n_new(CCollisionShape(new btCapsuleShapeZ(Radius, CylinderLength), Offset, Scaling));
}
//---------------------------------------------------------------------

PCollisionShape CCollisionShape::CreateConvexHull(const vector3* pVertices, UPTR VertexCount, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
{
	if (!pVertices || !VertexCount) return nullptr;

	return n_new(CCollisionShape(new btConvexHullShape(pVertices[0].v, VertexCount, sizeof(vector3)), Offset, Scaling));
}
//---------------------------------------------------------------------

}
