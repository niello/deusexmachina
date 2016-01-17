#pragma once
#ifndef __DEM_L1_RENDER_MESH_H__
#define __DEM_L1_RENDER_MESH_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Render/VertexBuffer.h>
#include <Render/IndexBuffer.h>

// Mesh represents complete geometry information of a 3D model. It stores vertex buffer,
// optional index buffer and a list of primitive groups (also known as mesh subsets).
// This class also implements LOD based on using different primitive groups.
// Renderers can determine desired LOD level, where 0 is the best, and request a mesh group
// for the given submesh at the given LOD via GetGroup(Idx, LOD). Since some groups are shared between
// multiple LODs, and some SubMeshes can have no group in a certain LOD, there is an additional
// mapping layer, pGroupLODMapping.

namespace Render
{

// Fill this in a resource loader and pass to the CMesh::Create()
struct CMeshInitData
{
	CVertexBuffer*		pVertexBuffer;
	CIndexBuffer*		pIndexBuffer;
	CPrimitiveGroup*	pMeshGroupData;	// Mesh groups and optional mapping data
	UPTR				SubMeshCount;
	UPTR				LODCount;
	UPTR				RealGroupCount;	// If UseMapping is false, must be SubMeshCount * LODCount (0 also defaults to this value)
	bool				UseMapping;
};

class CMesh: public Resources::CResourceObject
{
	__DeclareClass(CMesh);

protected:

	//???multiple VBs? if shared, store offset per VB! store vertex layout(s) for different use cases,
	//or make it completely external. Or use offset in SetVertexBuffer? But it may cause many
	//SetVertexBuffer calls, which is bad. If IB shared, store offset in VB (or patch in mesh groups,
	//but it is worse as it kills reusability when buffer offsets change)!
	PVertexBuffer		VB;
	PIndexBuffer		IB;

	DWORD				SubMeshCount;
	DWORD				LODCount;
	DWORD				GroupCount;

	// To maintain cache coherency and reduce number of allocations, these two are essentially one piece of memory,
	// where pGroups is at Offset = 0 and pGroupLODMapping is at Offset = sizeof(CPrimitiveGroup) * GroupCount
	// If direct mapping is used, GroupCount = SubMeshCount * LODCount and pGroupLODMapping = NULL
	CPrimitiveGroup*	pGroups;			// Real submesh data
	CPrimitiveGroup**	pGroupLODMapping;	// CArray2D<CPrimitiveGroup*>[LOD][SubMeshIndex]

public:

	CMesh(): pGroups(NULL), pGroupLODMapping(NULL) {}
	virtual ~CMesh() { Destroy(); }

	bool					Create(const CMeshInitData& InitData);
	void					Destroy();

	virtual bool			IsResourceValid() const { return !!pGroups; }

	PVertexBuffer			GetVertexBuffer() const { return VB; }
	PIndexBuffer			GetIndexBuffer() const { return IB; }
	DWORD					GetSubMeshCount() const { return SubMeshCount; }
	const CPrimitiveGroup*	GetGroup(DWORD SubMeshIdx, DWORD LOD = 0) const;
};

typedef Ptr<CMesh> PMesh;

// Can return NULL, which means that nothing sould be rendered
inline const CPrimitiveGroup* CMesh::GetGroup(DWORD SubMeshIdx, DWORD LOD) const
{
	n_assert(LOD < LODCount && SubMeshIdx < SubMeshCount);
	if (pGroupLODMapping) return pGroupLODMapping[LOD * SubMeshCount + SubMeshIdx];
	else if (pGroups) return pGroups + LOD * SubMeshCount + SubMeshIdx;
	else return NULL;
}
//---------------------------------------------------------------------

}

#endif
