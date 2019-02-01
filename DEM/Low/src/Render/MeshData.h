#pragma once
#include <Resources/ResourceObject.h>
#include <Render/VertexComponent.h>
#include <Render/RenderFwd.h>
#include <Data/Array.h>

// Mesh data is an engine resource that incapsulates all necessary data
// for a mesh creation. Data is stored in a RAM and can be loaded from
// different formats or be generated procedurally. Each GPU can create its
// own CMesh object from this data.

namespace Data
{
	typedef std::unique_ptr<class IRAMData> PRAMData;
}

namespace Render
{

class CMeshData: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

public:

	Data::PRAMData						VBData;
	Data::PRAMData						IBData;
	CArray<Render::CPrimitiveGroup>		Groups;
	CArray<Render::CVertexComponent>	VertexFormat;
	Render::EIndexType					IndexType = Render::Index_16;
	U32									VertexCount = 0;
	U32									IndexCount = 0;

	virtual ~CMeshData();

	virtual bool IsResourceValid() const { return VBData && VertexCount; }
};

typedef Ptr<CMeshData> PMeshData;

}
