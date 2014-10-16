#pragma once
#ifndef __DEM_L1_RENDER_MESH_H__
#define __DEM_L1_RENDER_MESH_H__

#include <Resources/Resource.h>
#include <Render/RenderFwd.h>
#include <Render/VertexBuffer.h>
#include <Render/IndexBuffer.h>
#include <Events/EventsFwd.h>
#include <Math/AABB.h>

// Mesh represents complete geometry information about a 3D model. It stores vertex buffer,
// index buffer (if required) and a list of primitive groups (also known as mesh subsets).
// This class also implements LOD based on using different primitive groups.
// Renderers can determine desired LOD level, where 0 is the best, and request a mesh group
// for the given submesh at the given LOD via GetGroup(Idx, LOD). Since some groups are shared between
// multiple LODs, and some SubMeshes can have no group in a certain LOD, there is an additional
// mapping layer, pGroupLODMapping.

//!!!fix Setup()/Unload() vs Create()/Destroy() for resources!

namespace Render
{

struct CMeshGroup
{
	DWORD				FirstVertex;
	DWORD				VertexCount;
	DWORD				FirstIndex;
	DWORD				IndexCount;
	EPrimitiveTopology	Topology;
	CAABB				AABB;
};

// Fill this in a resource loader and pass to the CMesh::Create()
struct CMeshInitData
{
	CVertexBuffer*	pVertexBuffer;
	CIndexBuffer*	pIndexBuffer;
	CMeshGroup*		pMeshGroupData;	// Optional mapping data, mesh groups
	DWORD			SubMeshCount;
	DWORD			LODCount;
	DWORD			RealGroupCount;	// If UseMapping is false, must be SubMeshCount * LODCount
	bool			UseMapping;
};

class CMesh: public Resources::CResource
{
	__DeclareClass(CMesh);

protected:

	//!!!if VB & IB are shared, need to store offset (and mb total size) here! Or patch mesh group offsets.
	PVertexBuffer	VB;
	PIndexBuffer	IB;

	DWORD			SubMeshCount;
	DWORD			LODCount;
	DWORD			GroupCount;

	// To maintain cache coherency, these two are essentially one piece of memory, where
	// pGroupLODMapping is at Offset = 0 and pGroups is at Offset = sizeof(CMeshGroup*) * SubMeshCount * LODCount
	// If pGroupLODMapping is NULL, pGroups is at Offset = 0 & direct mapping is used. See GetGroup().
	CMeshGroup**	pGroupLODMapping;	// CArray2D<CMeshGroup*>[LOD][SubMeshIndex]
	CMeshGroup*		pGroups;			// Real submesh data

	//???need?
	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CMesh(CStrID ID): CResource(ID), pGroupLODMapping(NULL), pGroups(NULL) {}
	virtual ~CMesh() { if (IsLoaded()) Unload(); }

	bool				Create(const CMeshInitData& InitData);
	virtual void		Unload();

	PVertexBuffer		GetVertexBuffer() const { return VB; }
	PIndexBuffer		GetIndexBuffer() const { return IB; }
	DWORD				GetSubMeshCount() const { return SubMeshCount; }
	const CMeshGroup*	GetGroup(DWORD SubMeshIdx, DWORD LOD = 0) const;
	//!!!GetSizeInBytes for RAM & VRAM!
};

typedef Ptr<CMesh> PMesh;

// Can return NULL, which means that nothing sould be rendered
const CMeshGroup* CMesh::GetGroup(DWORD SubMeshIdx, DWORD LOD) const
{
	n_assert(LOD < LODCount && SubMeshIdx < SubMeshCount);
	if (pGroupLODMapping) return pGroupLODMapping[LOD * SubMeshCount + SubMeshIdx];
	else if (pGroups) return pGroups + LOD * SubMeshCount + SubMeshIdx;
	else return NULL;
}
//---------------------------------------------------------------------

}

#endif
