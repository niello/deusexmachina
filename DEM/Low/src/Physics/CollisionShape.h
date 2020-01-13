#pragma once
#include <Resources/ResourceObject.h>
#include <Math/Vector3.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{
typedef Ptr<class CCollisionShape> PCollisionShape;

class CCollisionShape : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	btCollisionShape* pBtShape = nullptr;

public:

	static PCollisionShape CreateSphere(const vector3& Offset, float Radius);
	static PCollisionShape CreateBox(const vector3& Offset, const vector3& Size);
	static PCollisionShape CreateCapsuleY(const vector3& Offset, float Radius, float CylinderLength);
	// TODO: CreateMesh(Render::PMeshData MeshData)

	CCollisionShape(btCollisionShape* pShape);
	virtual ~CCollisionShape() override;

	virtual bool           IsResourceValid() const { return !!pBtShape; }
	virtual const vector3& GetOffset() const { return vector3::Zero; } // Could use btCompoundShape instead
	btCollisionShape*      GetBulletShape() const { return pBtShape; }
};

}
