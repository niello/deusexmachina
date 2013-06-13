#pragma once
#ifndef __DEM_L1_RENDER_MESH_H__
#define __DEM_L1_RENDER_MESH_H__

#include <Resources/Resource.h>
#include <Render/RenderFwd.h>
#include <Render/Geometry/VertexBuffer.h>
#include <Render/Geometry/IndexBuffer.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>
#include <mathlib/bbox.h>

// Mesh represents complete geometry information about a 3D model. It stores vertex buffer,
// index buffer (if required) and list of primitive groups (also known as mesh subsets).

namespace Render
{

struct CMeshGroup
{
	DWORD				FirstVertex;
	DWORD				VertexCount;
	DWORD				FirstIndex;
	DWORD				IndexCount;
	EPrimitiveTopology	Topology;
	bbox3				AABB;
};

class CMesh: public Resources::CResource
{
	__DeclareClass(CMesh);

protected:

	//!!!if VB & IB are shared, need to store offset (and mb total size) here!
	PVertexBuffer		VB;
	PIndexBuffer		IB;
	nArray<CMeshGroup>	Groups;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CMesh(CStrID ID): CResource(ID) {}
	virtual ~CMesh() { if (IsLoaded()) Unload(); }

	bool				Setup(CVertexBuffer* VertexBuffer, CIndexBuffer* IndexBuffer, const nArray<CMeshGroup>& MeshGroups);
	virtual void		Unload();

	PVertexBuffer		GetVertexBuffer() const { return VB; }
	PIndexBuffer		GetIndexBuffer() const { return IB; }
	const CMeshGroup&	GetGroup(DWORD Idx) const { return Groups[Idx]; }
};

typedef Ptr<CMesh> PMesh;

}

#endif
