#pragma once
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <acl/core/unique_ptr.h>
#include <acl/math/vector4_32.h>
#include <vector>
#include <map>
#include <filesystem>
#include <optional>

// Common tools for scene hierarchy and resources processing.
// Used by converters from different common formats (FBX, glTF) to DEM.

// At least 2 UVs and 4 bones according to the core glTF 2.0 spec:
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
constexpr size_t MaxUV = 4;
constexpr size_t MaxBonesPerVertex = 4;
constexpr float PI = 3.14159265358979323846f;

inline float RadToDeg(float Rad) { return Rad * 180.0f / PI; }
inline float DegToRad(float Deg) { return Deg * PI / 180.0f; }

class CThreadSafeLog;

namespace Data
{
	typedef std::map<CStringID, class CDataScheme> CSchemeSet;
}

struct CSceneSettings
{
	std::map<std::string, std::string>              EffectsByType;
	std::map<std::string, std::string, std::less<>> EffectParamAliases;

	std::string GetEffectParamID(const std::string_view Alias) const
	{
		auto It = EffectParamAliases.find(Alias);
		return (It == EffectParamAliases.cend()) ? std::string(Alias) : It->second;
	}
};

// TODO: look at fbx2acl for correct FBX animation export
namespace acl
{
	class IAllocator;
	class AnimationClip;
	typedef std::unique_ptr<class RigidSkeleton, Deleter<RigidSkeleton>> RigidSkeletonPtr;
	struct Transform_32;
}

struct CVertex
{
	//???use 32-bit ACL/RTM vectors?
	float3   Position;
	float3   Normal;
	float3   Tangent;
	float3   Bitangent;
	uint32_t Color; // RGBA
	float2   UV[MaxUV];
	size_t   BonesUsed = 0;
	int      BlendIndices[MaxBonesPerVertex];
	float    BlendWeights32[MaxBonesPerVertex] = { 0.f };
	uint16_t BlendWeights16[MaxBonesPerVertex] = { 0 };
	uint32_t BlendWeights8 = 0; // 4x uint8_t packed
};

struct CVertexFormat
{
	size_t NormalCount = 0;
	size_t TangentCount = 0;
	size_t BitangentCount = 0;
	size_t ColorCount = 0;
	size_t UVCount = 0;
	size_t BonesPerVertex = 0;
	size_t BlendWeightSize = 16;
};

struct CAABB
{
	float3 Min = { FLT_MAX, FLT_MAX, FLT_MAX };
	float3 Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
};

struct CMeshGroup
{
	std::vector<CVertex> Vertices;
	std::vector<unsigned int> Indices;
	CAABB AABB;
};

struct CMeshAttrInfo
{
	std::string MeshID;
	std::vector<std::string> MaterialIDs; // Per group (submesh)
	CAABB AABB;
};

struct CSkinAttrInfo
{
	std::string SkinID;
	std::string RootSearchPath;
};

constexpr uint16_t NoParentBone = static_cast<uint16_t>(-1);

// There are a couple of approaches for saving bones:
// 1. Save node names, ensure they are unique and search all the scene node tree recursively for them
// 2. Save node name and parent bone index, or full path if parent is not a bone
// 3. Hybrid - save node name and parent bone index. If no parent bone specified, search recursively like in 1.
// Approach 3 is used here.
struct CBone
{
	float       InvLocalBindPose[16];
	std::string ID;
	uint16_t    ParentBoneIndex = NoParentBone;
	// TODO: bone object-space or local-space AABB

	bool operator ==(const CBone& Other) const
	{
		if (ParentBoneIndex != Other.ParentBoneIndex || ID != Other.ID) return false;

		for (size_t i = 0; i < 16; ++i)
			if (std::fabs(InvLocalBindPose[i] - Other.InvLocalBindPose[i]) > 0.00001f)
				return false;

		return true;
	}
};

struct CLocomotionInfo
{
	uint32_t               CycleStartFrame = 0;
	uint32_t               LeftFootOnGroundFrame = std::numeric_limits<uint32_t>().max();
	uint32_t               RightFootOnGroundFrame = std::numeric_limits<uint32_t>().max();
	float                  SpeedFromRoot = 0.f;
	float                  SpeedFromFeet = 0.f;
	std::vector<float>     Phases;
	std::map<float, float> PhaseNormalizedTimes;
};

inline void NormalizeWeights32x4(float& w1, float& w2, float& w3, float& w4)
{
	const auto Sum = w1 + w2 + w3 + w4;
	w1 /= Sum;
	w2 /= Sum;
	w3 /= Sum;
	w4 /= Sum;
}
//---------------------------------------------------------------------

// FIXME: rounding error may lead to a sum != 65535
inline void NormalizeWeights16x4(uint16_t& w1, uint16_t& w2, uint16_t& w3, uint16_t& w4)
{
	float f1 = ShortToNormalizedFloat(w1);
	float f2 = ShortToNormalizedFloat(w2);
	float f3 = ShortToNormalizedFloat(w3);
	float f4 = ShortToNormalizedFloat(w4);
	NormalizeWeights32x4(f1, f2, f3, f4);
	w1 = NormalizedFloatToShort(f1);
	w2 = NormalizedFloatToShort(f2);
	w3 = NormalizedFloatToShort(f3);
	w4 = NormalizedFloatToShort(f4);
}
//---------------------------------------------------------------------

// FIXME: rounding error may lead to a sum != 255
inline void NormalizeWeights8x4(uint8_t& w1, uint8_t& w2, uint8_t& w3, uint8_t& w4)
{
	float f1 = ByteToNormalizedFloat(w1);
	float f2 = ByteToNormalizedFloat(w2);
	float f3 = ByteToNormalizedFloat(w3);
	float f4 = ByteToNormalizedFloat(w4);
	NormalizeWeights32x4(f1, f2, f3, f4);
	w1 = NormalizedFloatToByte(f1);
	w2 = NormalizedFloatToByte(f2);
	w3 = NormalizedFloatToByte(f3);
	w4 = NormalizedFloatToByte(f4);
}
//---------------------------------------------------------------------

inline void InitAABBWithVertex(CAABB& Dest, const float3& Vertex)
{
	Dest.Min.x = Vertex.x;
	Dest.Max.x = Vertex.x;
	Dest.Min.y = Vertex.y;
	Dest.Max.y = Vertex.y;
	Dest.Min.z = Vertex.z;
	Dest.Max.z = Vertex.z;
}
//---------------------------------------------------------------------

inline void ExtendAABB(CAABB& Dest, const float3& Vertex)
{
	if (Dest.Min.x > Vertex.x) Dest.Min.x = Vertex.x;
	else if (Dest.Max.x < Vertex.x) Dest.Max.x = Vertex.x;

	if (Dest.Min.y > Vertex.y) Dest.Min.y = Vertex.y;
	else if (Dest.Max.y < Vertex.y) Dest.Max.y = Vertex.y;

	if (Dest.Min.z > Vertex.z) Dest.Min.z = Vertex.z;
	else if (Dest.Max.z < Vertex.z) Dest.Max.z = Vertex.z;
}
//---------------------------------------------------------------------

inline void MergeAABBs(CAABB& Dest, const CAABB& Src)
{
	if (Dest.Min.x > Src.Min.x) Dest.Min.x = Src.Min.x;
	if (Dest.Min.y > Src.Min.y) Dest.Min.y = Src.Min.y;
	if (Dest.Min.z > Src.Min.z) Dest.Min.z = Src.Min.z;
	if (Dest.Max.x < Src.Max.x) Dest.Max.x = Src.Max.x;
	if (Dest.Max.y < Src.Max.y) Dest.Max.y = Src.Max.y;
	if (Dest.Max.z < Src.Max.z) Dest.Max.z = Src.Max.z;
}
//---------------------------------------------------------------------

std::string GetRelativeNodePath(std::vector<std::string>&& From, std::vector<std::string>&& To);
bool LoadSceneSettings(const std::filesystem::path& Path, CSceneSettings& Out);
void ProcessGeometry(const std::vector<CVertex>& RawVertices, const std::vector<unsigned int>& RawIndices, std::vector<CVertex>& Vertices, std::vector<unsigned int>& Indices);
void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex);
bool WriteDEMMesh(const std::filesystem::path& DestPath, const std::map<std::string, CMeshGroup>& SubMeshes, const CVertexFormat& VertexFormat, size_t BoneCount, CThreadSafeLog& Log);
bool ReadDEMMeshVertexPositions(const std::filesystem::path& Path, std::vector<float3>& Out, CThreadSafeLog& Log);
bool WriteDEMSkin(const std::filesystem::path& DestPath, const std::vector<CBone>& Bones, CThreadSafeLog& Log);
bool WriteDEMAnimation(const std::filesystem::path& DestPath, acl::IAllocator& ACLAllocator, const acl::AnimationClip& Clip, const std::vector<std::string>& NodeNames, const CLocomotionInfo* pLocomotionInfo, CThreadSafeLog& Log);
bool WriteDEMScene(const std::filesystem::path& DestDir, const std::string& Name, Data::CParams&& Nodes, const Data::CSchemeSet& Schemes, const Data::CParams& TaskParams, bool HRD, bool Binary, bool CreateRoot, CThreadSafeLog& Log);
void InitImageProcessing();
void TermImageProcessing();
std::string WriteTexture(const std::filesystem::path& SrcPath, const std::filesystem::path& DestDir, const Data::CParams& TaskParams, CThreadSafeLog& Log);
std::optional<std::string> GenerateCollisionShape(std::string ShapeType, const std::filesystem::path& ShapeDir, const std::string& MeshRsrcName, const CMeshAttrInfo& MeshInfo, const acl::Transform_32& GlobalTfm, const std::map<std::string, std::filesystem::path>& PathAliases, CThreadSafeLog& Log);
void FillNodeTransform(const acl::Transform_32& Tfm, Data::CParams& NodeSection);
bool ComputeLocomotion(CLocomotionInfo& Out, float FrameRate, acl::Vector4_32 ForwardDir, acl::Vector4_32 UpDir, acl::Vector4_32 SideDir, const std::vector<acl::Vector4_32>& RootPositions, const std::vector<acl::Vector4_32>& LeftFootPositions, const std::vector<acl::Vector4_32>& RightFootPositions);
