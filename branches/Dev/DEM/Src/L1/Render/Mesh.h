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

//!!!NEED LOD INFO!
// Optimal if LODs are just different index buffers and corresponding mesh groups
// LOD arrays may be defined irectly in a mesh group with a fixed array

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

class CMesh: public Resources::CResource
{
	__DeclareClass(CMesh);

protected:

	//!!!if VB & IB are shared, need to store offset (and mb total size) here! Or patch mesh group offsets.
	PVertexBuffer		VB;
	PIndexBuffer		IB;
	CArray<CMeshGroup>	Groups;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CMesh(CStrID ID): CResource(ID) {}
	virtual ~CMesh() { if (IsLoaded()) Unload(); }

	bool				Setup(CVertexBuffer* VertexBuffer, CIndexBuffer* IndexBuffer, const CArray<CMeshGroup>& MeshGroups);
	virtual void		Unload();

	PVertexBuffer		GetVertexBuffer() const { return VB; }
	PIndexBuffer		GetIndexBuffer() const { return IB; }
	const CMeshGroup&	GetGroup(DWORD Idx) const { return Groups[Idx]; }
};

typedef Ptr<CMesh> PMesh;

}

#endif
