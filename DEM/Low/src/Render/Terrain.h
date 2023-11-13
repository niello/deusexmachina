#pragma once
#include <Render/Renderable.h>
#include <Data/StringID.h>
#include <Data/Ptr.h>
#include <Math/CameraMath.h>
#include <array>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Render
{
typedef Ptr<class CCDLODData> PCDLODData;
typedef Ptr<class CMaterial> PMaterial;
typedef Ptr<class CMesh> PMesh;
typedef Ptr<class CTexture> PTexture;
class CLight;

class CTerrain : public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	using TMorton = U32;
	using TCellDim = U16; // Max 15 bits for quadtree which is more than enough for terrain subdivision

	struct CPatchInstance
	{
		rtm::vector4f        ScaleOffset;
		std::array<CLight*, 8> Lights;
		U32                    LightsVersion = 0;
		U32                    LOD;
		TMorton                MortonCode;
		bool                   IsFullPatch = false;
	};

	inline static constexpr U32 MAX_LIGHTS_PER_PATCH = std::tuple_size<decltype(CPatchInstance::Lights)>();

protected:

	enum class ENodeStatus
	{
		Invisible,
		NotInLOD,
		Processed
	};

	struct CNodeProcessingContext
	{
		Math::CSIMDFrustum ViewFrustum;
		rtm::vector4f    Scale;
		rtm::vector4f    Offset;
		vector3	           MainCameraPos;
	};

	std::vector<CPatchInstance> _Patches;
	std::vector<CPatchInstance> _PrevPatches;        // Stored here to avoid per frame allocations
	size_t                      _FullPatchCount = 0; // Number of full (not quarter) patches in _Patches. Used for minor optimization.

	//CB or CBs (can add here shadere param storage and make persistent buffers for patch data, get shader meta from saved tech)
	//!!!???can fill one GPU buffer?! use offset to render first ones, then others.
	//???store main info about patches in one buffer, lights in another?!
	// - will save buffer refreshes when only light data changes
	// - depth will bind only main data, no lights
	// - more data will fit into constant buffers w/out structured buffers
	// - maybe better layout
	// - different shaders use it! VS / PS.

	float                       _VisibilityRange = 0.f;

	ENodeStatus ProcessTerrainNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD, U8 ParentClipStatus, TMorton MortonCode);

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
	float                   MorphStartRatio = 0.7f;             // TODO: read from view settings? //!!!clamp to range 0.5f .. 0.95f!
	U16                     DeepestLOD = 0;                     // TODO: read from view settings?
	U16                     MaxLODForDynamicLights = 3;         // LOD 0 can't be disabled on intent. TODO: read from view settings?

	CTerrain();
	virtual ~CTerrain() override;

	void UpdateMorphConstants(float VisibilityRange);
	void UpdatePatches(const vector3& MainCameraPos, const Math::CSIMDFrustum& ViewFrustum);

	CCDLODData*         GetCDLODData() const { return CDLODData.Get(); }
	CMaterial*          GetMaterial() const { return Material.Get(); }
	CTexture*           GetHeightMap() const { return HeightMap.Get(); }
	const auto&         GetPatches() const { return _Patches; }
	auto&               GetPatches() { return _Patches; }
	size_t              GetFullPatchCount() const { return _FullPatchCount; }
	CMesh*              GetPatchMesh() const { return PatchMesh.Get(); }
	CMesh*              GetQuarterPatchMesh() const { return QuarterPatchMesh.Get(); }
	float               GetInvSplatSizeX() const { return InvSplatSizeX; }
	float               GetInvSplatSizeZ() const { return InvSplatSizeZ; }
	float               GetVisibilityRange() const { return _VisibilityRange; }
};

typedef Ptr<CTerrain> PTerrain;

}
