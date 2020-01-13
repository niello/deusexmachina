#pragma once
#include <Resources/ResourceObject.h>
#include <Math/Vector3.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{

class CCollisionShape : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	btCollisionShape* pBtShape = nullptr;

public:

	CCollisionShape(btCollisionShape* pShape);
	virtual ~CCollisionShape() override;

	virtual bool           IsResourceValid() const { return !!pBtShape; }
	virtual const vector3& GetOffset() const { return vector3::Zero; } // Could use btCompoundShape instead
	btCollisionShape*      GetBulletShape() const { return pBtShape; }
};

typedef Ptr<CCollisionShape> PCollisionShape;

}
