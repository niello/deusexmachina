#pragma once
#ifndef __DEM_L1_RENDER_MESH_H__
#define __DEM_L1_RENDER_MESH_H__

#include <Resources/Resource.h>
#include <Render/Geometry/VertexBuffer.h>
#include <Render/Geometry/IndexBuffer.h>
#include <mathlib/bbox.h>

// Mesh represents complete geometry information about a 3D model. It stores vertex buffer,
// index buffer (if required) and list of primitive groups (also known as mesh subsets).

namespace Render
{

enum EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriList,
	TriStrip
};

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
protected:

	//!!!if VB & IB are shared, need to store offset (and mb total size) here!
	PVertexBuffer		VB;
	PIndexBuffer		IB;
	nArray<CMeshGroup>	Groups;

public:

	CMesh(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost) {}
};

typedef Ptr<CMesh> PMesh;

}

#endif
