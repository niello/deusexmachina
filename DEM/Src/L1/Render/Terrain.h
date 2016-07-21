#pragma once
#ifndef __DEM_L1_FRAME_TERRAIN_H__
#define __DEM_L1_FRAME_TERRAIN_H__

#include <Render/Renderable.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Resources
{
	typedef Ptr<class CResource> PResource;
}

namespace Render
{

class CTerrain: public IRenderable
{
	__DeclareClass(CTerrain);

protected:

	Resources::PResource	RCDLODData;
	PCDLODData				CDLODData;	//???NEED?
	Resources::PResource	RMaterial;
	PMaterial				Material;	//???NEED?
	PMesh					PatchMesh;
	PMesh					QuarterPatchMesh;

	float					InvSplatSizeX;
	float					InvSplatSizeZ;

	virtual bool	ValidateResources(PGPUDriver GPU);

public:

	CTerrain(): InvSplatSizeX(0.1f), InvSplatSizeZ(0.1f) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual bool	GetLocalAABB(CAABB& OutBox, UPTR LOD) const { OutBox = CDLODData->GetAABB(); OK; }

	Render::CMesh*	GetPatchMesh() const { return PatchMesh.GetUnsafe(); }
	Render::CMesh*	GetQuarterPatchMesh() const { return QuarterPatchMesh.GetUnsafe(); }
	float			GetInvSplatSizeX() const { return InvSplatSizeX; }
	float			GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

}

#endif
