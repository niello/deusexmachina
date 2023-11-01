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

protected:

	// Vertex shader instance data
	struct alignas(16) CPatchInstance
	{
		float ScaleOffset[4];
		float MorphConsts[2];
		float _PAD1[2];
	};

	// Pixel shader instance data
	//!!!can store as a simple unstructured buffer of indices!
	//I16 LightIndex[INSTANCE_MAX_LIGHT_COUNT]; // + INSTANCE_PADDING_SIZE];

	enum class ENodeStatus
	{
		Invisible,
		NotInLOD,
		Processed
	};

	struct CNodeProcessingContext
	{
		matrix44 ViewProjection;
		vector3	 MainCameraPos;
		float    AABBMinX;
		float    AABBMinZ;
		vector3  ScaleBase;
	};

	//!!!???can fill one GPU buffer?! use offset to render first ones, then others.
	std::vector<CPatchInstance> _Patches;
	std::vector<CPatchInstance> _QuarterPatches;

	ENodeStatus ProcessTerrainNode(const CNodeProcessingContext& Ctx, U32 X, U32 Z, U32 LOD, EClipStatus Clip);

public:

	struct CLODParams
	{
		float Range;
		float Morph1;
		float Morph2;
	};

	PCDLODData				CDLODData;
	PMaterial				Material;
	PTexture				HeightMap;
	PMesh					PatchMesh;
	PMesh					QuarterPatchMesh;
	std::vector<CLODParams> LODParams;

	float					InvSplatSizeX = 1.f;
	float					InvSplatSizeZ = 1.f;

	U32                     ShaderTechIndex = INVALID_INDEX_T<U32>;
	U32                     PatchesTransformVersion = 0;
	float                   VisibilityRange = 0.f;

	CTerrain();
	virtual ~CTerrain() override;

	void UpdatePatches(const vector3& MainCameraPos, const matrix44& ViewProjection);

	CCDLODData*         GetCDLODData() const { return CDLODData.Get(); }
	CMaterial*          GetMaterial() const { return Material.Get(); }
	CTexture*           GetHeightMap() const { return HeightMap.Get(); }
	const auto&         GetPatches() const { return _Patches; }
	const auto&         GetQuarterPatches() const { return _QuarterPatches; }
	CMesh*              GetPatchMesh() const { return PatchMesh.Get(); }
	CMesh*              GetQuarterPatchMesh() const { return QuarterPatchMesh.Get(); }
	float               GetInvSplatSizeX() const { return InvSplatSizeX; }
	float               GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

}
