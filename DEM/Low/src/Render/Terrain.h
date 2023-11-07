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

class CTerrain : public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	using TMorton = U32;
	using TCellDim = U16; // Max 15 bits for quadtree which is more than enough for terrain subdivision

	struct alignas(16) CPatchInstance
	{
		acl::Vector4_32    ScaleOffset;
		std::array<U32, 8> GPULightIndices;
		U32                LightsVersion = 0;
		U32                LOD;
		TMorton            MortonCode;
	};

	inline static constexpr U32 MAX_LIGHTS_PER_PATCH = std::tuple_size<decltype(CPatchInstance::GPULightIndices)>();

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
		acl::Vector4_32    Scale;
		acl::Vector4_32    Offset;
		vector3	           MainCameraPos;
	};

	std::vector<CPatchInstance> _Patches;
	std::vector<CPatchInstance> _QuarterPatches;
	std::vector<CPatchInstance> _PrevPatches;        // stored here to avoid per frame allocations
	std::vector<CPatchInstance> _PrevQuarterPatches; // stored here to avoid per frame allocations

	//CB or CBs
	//!!!???can fill one GPU buffer?! use offset to render first ones, then others.
	//???store main info about patches in one buffer, lights in another?!
	// - will save buffer refreshes when only light data changes
	// - depth will bind only main data, no lights
	// - more data will fit into constant buffers w/out structured buffers
	// - maybe better layout
	// - different shaders use it! VS / PS.

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
	float                   VisibilityRange = 0.f;
	U16                     MaxLODForDynamicLights = 3;         // LOD 0 can't be disabled on intent. TODO: read from view settings?

	CTerrain();
	virtual ~CTerrain() override;

	void UpdatePatches(const vector3& MainCameraPos, const Math::CSIMDFrustum& ViewFrustum);

	CCDLODData*         GetCDLODData() const { return CDLODData.Get(); }
	CMaterial*          GetMaterial() const { return Material.Get(); }
	CTexture*           GetHeightMap() const { return HeightMap.Get(); }
	const auto&         GetPatches() const { return _Patches; }
	const auto&         GetQuarterPatches() const { return _QuarterPatches; }
	auto&               GetPatches() { return _Patches; }
	auto&               GetQuarterPatches() { return _QuarterPatches; }
	CMesh*              GetPatchMesh() const { return PatchMesh.Get(); }
	CMesh*              GetQuarterPatchMesh() const { return QuarterPatchMesh.Get(); }
	float               GetInvSplatSizeX() const { return InvSplatSizeX; }
	float               GetInvSplatSizeZ() const { return InvSplatSizeZ; }
};

typedef Ptr<CTerrain> PTerrain;

}
