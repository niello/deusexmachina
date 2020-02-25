#pragma once
#include <Resources/ResourceObject.h>
#include <Render/VertexComponent.h>
#include <Render/RenderFwd.h>
#include <Data/Array.h>

// Mesh represents complete geometry information of a 3D model. It stores vertex data,
// optional index data and a list of primitive groups (also known as mesh subsets).
// This class also implements LOD based on using different primitive groups.
// Renderers can determine desired LOD level, where 0 is the best, and request a mesh group
// for the given submesh at the given LOD via GetGroup(Idx, LOD). Since some groups are shared between
// multiple LODs, and some SubMeshes can have no group in a certain LOD, there is an additional
// mapping layer, pGroupLODMapping.

namespace Data
{
	typedef std::unique_ptr<class IBuffer> PBuffer;
}

namespace Render
{

class CMeshData: public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	// To maintain cache coherency and reduce number of allocations, these two are essentially one piece of memory,
	// where pGroups is at Offset = 0 and pGroupLODMapping is at Offset = sizeof(CPrimitiveGroup) * GroupCount
	// If direct mapping is used, GroupCount = SubMeshCount * LODCount and pGroupLODMapping = nullptr
	CPrimitiveGroup*					pGroups = nullptr;			// Real submesh data
	CPrimitiveGroup**					pGroupLODMapping = nullptr;	// CArray2D<CPrimitiveGroup*>[LOD][SubMeshIndex]

	UPTR								_SubMeshCount = 0;
	UPTR								_LODCount = 0;
	UPTR								_GroupCount = 0;

	UPTR								BufferUseCounter = 0;

public:

	Data::PBuffer						VBData;
	Data::PBuffer						IBData;
	std::vector<Render::CVertexComponent> VertexFormat;
	Render::EIndexType					IndexType = Render::Index_16;
	U32									VertexCount = 0;
	U32									IndexCount = 0;

	CMeshData();
	virtual ~CMeshData();

	virtual bool			IsResourceValid() const { return VBData && VertexCount; }
	void					Destroy();

	void					InitGroups(CPrimitiveGroup* pData, UPTR Count, UPTR SubMeshCount, UPTR LODCount, bool UseMapping, bool UpdateAABBs);
	const CPrimitiveGroup*	GetGroup(UPTR SubMeshIdx, UPTR LOD = 0) const;
	UPTR					GetSubMeshCount() const { return _SubMeshCount; }
	UPTR					GetLODCount() const { return _LODCount; }

	UPTR                    GetVertexSize() const;

	// Controls RAM texture data lifetime. Some GPU resources may want to keep this data in RAM.
	bool					UseBuffer();
	void					ReleaseBuffer();
};

typedef Ptr<CMeshData> PMeshData;

// Can return nullptr, which means that nothing sould be rendered
inline const CPrimitiveGroup* CMeshData::GetGroup(UPTR SubMeshIdx, UPTR LOD) const
{
	n_assert(LOD < _LODCount && SubMeshIdx < _SubMeshCount);
	if (pGroupLODMapping) return pGroupLODMapping[LOD * _SubMeshCount + SubMeshIdx];
	else if (pGroups) return pGroups + LOD * _SubMeshCount + SubMeshIdx;
	else return nullptr;
}
//---------------------------------------------------------------------

}
