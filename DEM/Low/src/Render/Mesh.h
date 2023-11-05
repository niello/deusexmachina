#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Render/VertexBuffer.h>
#include <Render/IndexBuffer.h>

// GPU mesh resource. It's VB & IB data reside in VRAM in most cases.

namespace Render
{
typedef Ptr<class CMeshData> PMeshData;

class CMesh: public ::Core::CObject
{
	FACTORY_CLASS_DECL;

protected:

	CStrID          _UID;
	PMeshData		_MeshData;
	PVertexBuffer	_VB;
	PIndexBuffer	_IB;
	U32             _SortingKey;

	// For sharing GPU buffers between multiple meshes
	//UPTR VBOffset;
	//UPTR IBOffset;

	bool _HoldRAMBackingData = false;

public:

	CMesh();
	virtual ~CMesh();

	bool					Create(CStrID UID, U16 SortingKey, PMeshData Data, PVertexBuffer VertexBuffer, PIndexBuffer IndexBuffer, bool HoldRAMCopy = false);
	void					Destroy();

	CStrID                  GetUID() const { return _UID; }
	const PMeshData&        GetMeshData() const { return _MeshData; }
	const PVertexBuffer&    GetVertexBuffer() const { return _VB; }
	const PIndexBuffer&     GetIndexBuffer() const { return _IB; }
	const CPrimitiveGroup*  GetGroup(UPTR SubMeshIdx, UPTR LOD = 0) const;
	U16                     GetSortingKey() const { return _SortingKey; }
};

typedef Ptr<CMesh> PMesh;

}
