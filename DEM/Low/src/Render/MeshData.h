#pragma once
#include <Resources/ResourceObject.h>
#include <Render/VertexComponent.h>
#include <Render/RenderFwd.h>
#include <Data/Array.h>

// Mesh data is an engine resource that incapsulates all necessary data
// for a mesh creation. Data is stored in a RAM and can be loaded from
// different formats or be generated procedurally. Each GPU can create its
// own CMesh object from this data.

// NB: for now data must be allocated by n_malloc_aligned(16).
//???use MMF? redesign meshes and textures?

namespace Render
{

class CMeshData: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

public:

	// If stream is valid, pData is a mapped stream contents. Else pData must be n_free'd.
	void*								pVBData = nullptr;
	void*								pIBData = nullptr;

	CArray<Render::CPrimitiveGroup>		Groups;
	CArray<Render::CVertexComponent>	VertexFormat;
	Render::EIndexType					IndexType = Render::Index_16;
	U32									VertexCount = 0;
	U32									IndexCount = 0;

	virtual ~CMeshData();

	virtual bool IsResourceValid() const { return pVBData && VertexCount; }
};

typedef Ptr<CMeshData> PMeshData;

}
