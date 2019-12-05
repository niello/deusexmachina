#pragma once
#include <Render/Renderable.h>
#include <Data/StringID.h>
#include <Data/Ptr.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Render
{
typedef Ptr<class CCDLODData> PCDLODData;
typedef Ptr<class CMaterial> PMaterial;
typedef Ptr<class CMesh> PMesh;
typedef Ptr<class CTexture> PTexture;

class CTerrain: public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PCDLODData				CDLODData;
	PMaterial				Material;
	PTexture				HeightMap;
	PMesh					PatchMesh;
	PMesh					QuarterPatchMesh;

	float					InvSplatSizeX = 0.1f;
	float					InvSplatSizeZ = 0.1f;

	CTerrain();
	virtual ~CTerrain();

	virtual PRenderable Clone() override;
	virtual bool        GetLocalAABB(CAABB& OutBox, UPTR LOD) const override;

	CCDLODData*         GetCDLODData() const { return CDLODData.Get(); }
	CMaterial*          GetMaterial() const { return Material.Get(); }
	CTexture*           GetHeightMap() const { return HeightMap.Get(); }
	CMesh*              GetPatchMesh() const { return PatchMesh.Get(); }
	CMesh*              GetQuarterPatchMesh() const { return QuarterPatchMesh.Get(); }
	float               GetInvSplatSizeX() const { return InvSplatSizeX; }
	float               GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

}
