#pragma once
#ifndef __DEM_L1_COLLISION_SHAPE_H__
#define __DEM_L1_COLLISION_SHAPE_H__

#include <Resources/ResourceObject.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{

class CCollisionShape: public Resources::CResourceObject
{
	__DeclareClass(CCollisionShape);

protected:

	btCollisionShape*	pBtShape;

public:

	CCollisionShape(): pBtShape(NULL) {}
	virtual ~CCollisionShape();// { if (IsLoaded()) Unload(); }

	bool				Setup(btCollisionShape* pShape);
	virtual void		Unload();
	virtual bool		IsResourceValid() const { FAIL; }
	virtual bool		GetOffset(vector3& Out) const { FAIL; }
	btCollisionShape*	GetBtShape() const { return pBtShape; }
};

typedef Ptr<CCollisionShape> PCollisionShape;

}

#endif
