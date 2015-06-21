#pragma once
#ifndef __DEM_L1_RENDER_MESH_H__
#define __DEM_L1_RENDER_MESH_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Render/VertexBuffer.h>
#include <Render/IndexBuffer.h>
#include <Events/EventsFwd.h>
#include <Math/AABB.h>

// Mesh represents complete geometry information of a 3D model. It stores vertex buffer,
// optional index buffer and a list of primitive groups (also known as mesh subsets).
// This class also implements LOD based on using different primitive groups.
// Renderers can determine desired LOD level, where 0 is the best, and request a mesh group
// for the given submesh at the given LOD via GetGroup(Idx, LOD). Since some groups are shared between
// multiple LODs, and some SubMeshes can have no group in a certain LOD, there is an additional
// mapping layer, pGroupLODMapping.

//!!!fix Setup()[Load()]/Unload() vs Create()/Destroy() for resources!

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
//!!!if VB & IB are shared and mesh starts not at buffers start, need to patch mesh group offsets in loader.
struct CMeshInitData
{
	CVertexBuffer*	pVertexBuffer;
	CIndexBuffer*	pIndexBuffer;
	CMeshGroup*		pMeshGroupData;	// Mesh groups and optional mapping data
	DWORD			SubMeshCount;
	DWORD			LODCount;
	DWORD			RealGroupCount;	// If UseMapping is false, must be SubMeshCount * LODCount (0 also defaults to this value)
	bool			UseMapping;
};

class CMesh: public Resources::CResourceObject
{
	__DeclareClass(CMesh);

protected:

	PVertexBuffer	VB;
	PIndexBuffer	IB;

	DWORD			SubMeshCount;
	DWORD			LODCount;
	DWORD			GroupCount;

	// To maintain cache coherency, these two are essentially one piece of memory, where
	// pGroups is at Offset = 0 and pGroupLODMapping is at Offset = sizeof(CMeshGroup) * GroupCount
	// If direct mapping is used, GroupCount = SubMeshCount * LODCount and pGroupLODMapping = NULL
	CMeshGroup*		pGroups;			// Real submesh data
	CMeshGroup**	pGroupLODMapping;	// CArray2D<CMeshGroup*>[LOD][SubMeshIndex]

public:

	CMesh(): pGroups(NULL), pGroupLODMapping(NULL) {}
	virtual ~CMesh() { Unload(); }

	bool				Create(const CMeshInitData& InitData);
	void				Unload();

	virtual bool		IsResourceValid() const { FAIL; }

	PVertexBuffer		GetVertexBuffer() const { return VB; }
	PIndexBuffer		GetIndexBuffer() const { return IB; }
	DWORD				GetSubMeshCount() const { return SubMeshCount; }
	const CMeshGroup*	GetGroup(DWORD SubMeshIdx, DWORD LOD = 0) const;
};

typedef Ptr<CMesh> PMesh;

// Can return NULL, which means that nothing sould be rendered
inline const CMeshGroup* CMesh::GetGroup(DWORD SubMeshIdx, DWORD LOD) const
{
	n_assert(LOD < LODCount && SubMeshIdx < SubMeshCount);
	if (pGroupLODMapping) return pGroupLODMapping[LOD * SubMeshCount + SubMeshIdx];
	else if (pGroups) return pGroups + LOD * SubMeshCount + SubMeshIdx;
	else return NULL;
}
//---------------------------------------------------------------------

}

#endif
