#pragma once
#ifndef __DEM_L1_STATIC_MESH_SHAPE_H__
#define __DEM_L1_STATIC_MESH_SHAPE_H__

#include <Physics/CollisionShape.h>

// Collision shape, that represents a static triangle mesh which can't change.
// It is very good for a static level geometry.

namespace Physics
{

class CStaticMeshShape: public CCollisionShape
{
	__DeclareClass(CStaticMeshShape);

protected:

	//btStridingMeshInterface* pMeshIface; //???store here? write some implementation with embedded storage? 

public:

	virtual ~CStaticMeshShape();// { if (IsLoaded()) Unload(); }

	//???can I get mesh iface from the shape itself?
	//bool			Setup(btBvhTriangleMeshShape* pShape, btStridingMeshInterface* pIMesh);
	//virtual void	Unload();
};

typedef Ptr<CStaticMeshShape> PStaticMeshShape;

}

#endif
