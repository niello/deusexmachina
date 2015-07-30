#pragma once
#ifndef __DEM_L1_SCENE_TERRAIN_H__
#define __DEM_L1_SCENE_TERRAIN_H__

#include <Render/RenderObject.h>
//#include <Render/Materials/ShaderVar.h>
#include <Render/Mesh.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Render
{

class CTerrain: public CRenderObject
{
	__DeclareClass(CTerrain);

protected:

	DWORD				HFWidth;
	DWORD				HFHeight;
	DWORD				PatchSize;
	DWORD				LODCount;
	DWORD				TopPatchCountX;
	DWORD				TopPatchCountZ;
	float				VerticalScale;
	CAABB				Box;

	struct CMinMaxMap
	{
		DWORD	PatchesW;
		DWORD	PatchesH;
		short*	pData;
	};

	short*				pMinMaxData;
	CArray<CMinMaxMap>	MinMaxMaps;

	PMesh				PatchMesh;
	PMesh				QuarterPatchMesh;
	PTexture			HeightMap;

	float				InvSplatSizeX;
	float				InvSplatSizeZ;

	CSPS*				pSPS;

	virtual void		OnDetachFromNode();
	virtual bool		ValidateResources();
	CMesh*				GetPatchMesh(DWORD Size);

public:

	//Render::CShaderVarMap	ShaderVars;

	CTerrain(): MinMaxMaps(2, 1), pMinMaxData(NULL), InvSplatSizeX(0.1f), InvSplatSizeZ(0.1f), pSPS(NULL) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);

	virtual void	UpdateInSPS(CSPS& SPS);
	const CAABB&	GetLocalAABB() const { return Box; }
	void			GetGlobalAABB(CAABB& Out) const;

	DWORD			GetHeightMapWidth() const { return HFWidth; }
	DWORD			GetHeightMapHeight() const { return HFHeight; }
	DWORD			GetPatchSize() const { return PatchSize; }
	DWORD			GetLODCount() const { return LODCount; }
	DWORD			GetTopPatchCountX() const { return TopPatchCountX; }
	DWORD			GetTopPatchCountZ() const { return TopPatchCountZ; }
	float			GetVerticalScale() const { return VerticalScale; }
	CMesh*			GetPatchMesh() const { return PatchMesh.GetUnsafe(); }
	CMesh*			GetQuarterPatchMesh() const { return QuarterPatchMesh.GetUnsafe(); }
	CTexture*		GetHeightMap() const { return HeightMap.GetUnsafe(); }
	void			GetMinMaxHeight(DWORD X, DWORD Z, DWORD LOD, short& MinY, short& MaxY) const;
	bool			HasNode(DWORD X, DWORD Z, DWORD LOD) const;
	float			GetInvSplatSizeX() const { return InvSplatSizeX; }
	float			GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

inline void CTerrain::GetMinMaxHeight(DWORD X, DWORD Z, DWORD LOD, short& MinY, short& MaxY) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	const short* pMMRec = MMMap.pData + ((Z * MMMap.PatchesW + X) << 1);
	MinY = *pMMRec;
	MaxY = *(pMMRec + 1);
}
//---------------------------------------------------------------------

inline bool CTerrain::HasNode(DWORD X, DWORD Z, DWORD LOD) const
{
	CMinMaxMap& MMMap = MinMaxMaps[LOD];
	return X < MMMap.PatchesW && Z < MMMap.PatchesH;
}
//---------------------------------------------------------------------

}

#endif
