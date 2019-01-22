#pragma once
#include <Render/Renderable.h>
#include <Data/StringID.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Resources
{
	typedef Ptr<class CResource> PResource;
}

namespace Render
{
typedef Ptr<class CCDLODData> PCDLODData;
typedef Ptr<class CMaterial> PMaterial;
typedef Ptr<class CMesh> PMesh;
typedef Ptr<class CTexture> PTexture;

class CTerrain: public IRenderable
{
	__DeclareClass(CTerrain);

protected:

	Resources::PResource	RCDLODData;
	PCDLODData				CDLODData;	//???NEED?
	CStrID					MaterialUID;
	PMaterial				Material;
	CStrID					HeightMapUID;
	PTexture				HeightMap;
	PMesh					PatchMesh;
	PMesh					QuarterPatchMesh;

	float					InvSplatSizeX = 0.1f;
	float					InvSplatSizeZ = 0.1f;

	virtual bool			ValidateResources(CGPUDriver* pGPU);

public:

	CTerrain();
	virtual ~CTerrain();

	virtual bool			LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual IRenderable*	Clone();
	virtual bool			GetLocalAABB(CAABB& OutBox, UPTR LOD) const;

	CCDLODData*				GetCDLODData() const { return CDLODData.Get(); }
	CMaterial*				GetMaterial() const { return Material.Get(); }
	CTexture*				GetHeightMap() const { return HeightMap.Get(); }
	CMesh*					GetPatchMesh() const { return PatchMesh.Get(); }
	CMesh*					GetQuarterPatchMesh() const { return QuarterPatchMesh.Get(); }
	float					GetInvSplatSizeX() const { return InvSplatSizeX; }
	float					GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

}
