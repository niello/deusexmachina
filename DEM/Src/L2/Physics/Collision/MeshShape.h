#pragma once
#ifndef __DEM_L2_PHYS_MESH_SHAPE_H__
#define __DEM_L2_PHYS_MESH_SHAPE_H__

#include "Shape.h"
//#include <mathlib/sphere.h>
//#include <mathlib/line.h>
//#ifdef MAX_SDWORD
//#undef MAX_SDWORD
//#endif
//#define BAN_OPCODE_AUTOLINK
//#include <opcode/Opcode.h>

// An shape containing a triangle mesh. Can be initialized from in-memory nMesh2.
// Based on CMeshShape (C) 2003 RadonLabs GmbH and A.Sysoev's tutorial

namespace Physics
{

class CMeshShape: public CShape
{
	__DeclareClass(CMeshShape);

protected:

	friend class CRay;

	//static Opcode::SphereCollider	OPCSphereCollider;	// an OPCODE sphere collider
	//Opcode::SphereCache				OPCSphereCache;		// an OPCODE sphere cache
	//static Opcode::RayCollider		OPCRayCollider;		// an OPCODE ray collider
	//static Opcode::CollisionFaces	OPCFaces;			// face cache for ray collider

	nString			FileName;
	float*			pVBuffer;
	int*			pIBuffer;
	int				VertexCount;
	int				VertexWidth;
	int				IndexCount;
	dTriMeshDataID	ODETriMeshDataID;
	dGeomID			ODETriMeshID;

public:

	CMeshShape();
	virtual ~CMeshShape();

	virtual void	Init(Data::PParams Desc);
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	Detach();
	virtual void	RenderDebug(const matrix44& ParentTfm);

	//int				DoSphereCollide(const sphere& Sphere, uint*& OutFaceIndices);
	//bool			DoRayCheck(const line3& Line, vector3& Contact);

	//void			SetMesh(nMesh2* pMesh) { n_assert(pMesh->IsValid()); pInitMesh = pMesh; }
	void			SetFileName(const nString& Name) { FileName = Name; }
	const nString&	GetFileName() const { return FileName; }
	const float*	GetVertexBuffer() const { return pVBuffer; }
	const int*		GetIndexBuffer() const { return pIBuffer; }
	int				GetNumVertices() const { return VertexCount; }
	int				GetVertexWidth() const { return VertexWidth; }
	int				GetNumIndices() const { return IndexCount; }
};

__RegisterClassInFactory(CMeshShape);

}

#endif
