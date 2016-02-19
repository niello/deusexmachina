#pragma once
#ifndef __DEM_L1_FRAME_TERRAIN_H__
#define __DEM_L1_FRAME_TERRAIN_H__

#include <Render/Renderable.h>
//#include <Render/Materials/ShaderVar.h>
#include <Render/Mesh.h>
#include <Data/Array.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Render
{

class CTerrain: public IRenderable
{
	__DeclareClass(CTerrain);

protected:

	UPTR				HFWidth;
	UPTR				HFHeight;
	UPTR				PatchSize;
	UPTR				LODCount;
	UPTR				TopPatchCountX;
	UPTR				TopPatchCountZ;
	float				VerticalScale;
	CAABB				Box;

	struct CMinMaxMap
	{
		UPTR	PatchesW;
		UPTR	PatchesH;
		short*	pData;
	};

	short*				pMinMaxData;
	CArray<CMinMaxMap>	MinMaxMaps;

	PMesh				PatchMesh;
	PMesh				QuarterPatchMesh;
	PTexture			HeightMap;
	CStrID				HeightMapUID; //!!!DBG TMP!

	float				InvSplatSizeX;
	float				InvSplatSizeZ;

	virtual bool		ValidateResources();
	CMesh*				GetPatchMesh(UPTR Size);

public:

	//Render::CShaderVarMap	ShaderVars;

	CTerrain(): MinMaxMaps(2, 1), pMinMaxData(NULL), InvSplatSizeX(0.1f), InvSplatSizeZ(0.1f) {}

	virtual bool		LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual bool		GetLocalAABB(CAABB& OutBox, UPTR LOD) const { OutBox = Box; OK; }

	UPTR				GetHeightMapWidth() const { return HFWidth; }
	UPTR				GetHeightMapHeight() const { return HFHeight; }
	UPTR				GetPatchSize() const { return PatchSize; }
	UPTR				GetLODCount() const { return LODCount; }
	UPTR				GetTopPatchCountX() const { return TopPatchCountX; }
	UPTR				GetTopPatchCountZ() const { return TopPatchCountZ; }
	float				GetVerticalScale() const { return VerticalScale; }
	Render::CMesh*		GetPatchMesh() const { return PatchMesh.GetUnsafe(); }
	Render::CMesh*		GetQuarterPatchMesh() const { return QuarterPatchMesh.GetUnsafe(); }
	Render::CTexture*	GetHeightMap() const { return HeightMap.GetUnsafe(); }
	void				GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, short& MinY, short& MaxY) const;
	bool				HasNode(UPTR X, UPTR Z, UPTR LOD) const;
	float				GetInvSplatSizeX() const { return InvSplatSizeX; }
	float				GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

inline void CTerrain::GetMinMaxHeight(UPTR X, UPTR Z, UPTR LOD, short& MinY, short& MaxY) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	const short* pMMRec = MMMap.pData + ((Z * MMMap.PatchesW + X) << 1);
	MinY = *pMMRec;
	MaxY = *(pMMRec + 1);
}
//---------------------------------------------------------------------

inline bool CTerrain::HasNode(UPTR X, UPTR Z, UPTR LOD) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

}

#endif
