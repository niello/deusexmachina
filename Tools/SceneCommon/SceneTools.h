#pragma once
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <vector>
#include <map>
#include <filesystem>

// Common tools for scene hierarchy and resources processing.
// Used by converters from different common formats (FBX, glTF) to DEM.

// At least 2 UVs and 4 bones according to the core glTF 2.0 spec:
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
constexpr size_t MaxUV = 4;
constexpr size_t MaxBonesPerVertex = 4;

inline float RadToDeg(float Rad) { return Rad * 180.0f / 3.1415926535897932385f; }
inline float DegToRad(float Deg) { return Deg * 3.1415926535897932385f / 180.0f; }

class CThreadSafeLog;

//???use 32-bit ACL/RTM vectors?
struct float2
{
	union
	{
		float v[2];
		struct
		{
			float x, y;
		};
	};

	constexpr float2() : x(0.f), y(0.f) {}
	constexpr float2(float x_, float y_) : x(x_), y(y_) {}

	bool operator ==(const float2& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y); }
	bool operator !=(const float2& Other) const { return !(*this == Other); }
};

//???use 32-bit ACL/RTM vectors?
struct float3
{
	union
	{
		float v[3];
		struct
		{
			float x, y, z;
		};
	};

	constexpr float3() : x(0.f), y(0.f), z(0.f) {}
	constexpr float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	bool operator ==(const float3& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y) && CompareFloat(z, Other.z); }
	bool operator !=(const float3& Other) const { return !(*this == Other); }
};

//???use 32-bit ACL/RTM vectors?
struct float4
{
	union
	{
		float v[4];
		struct
		{
			float x, y, z, w;
		};
	};

	constexpr float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
	constexpr float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

	bool operator ==(const float4& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y) && CompareFloat(z, Other.z) && CompareFloat(w, Other.w); }
	bool operator !=(const float4& Other) const { return !(*this == Other); }
};

struct CVertex
{
	//???use 32-bit ACL/RTM vectors?
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float3 Bitangent;
	uint32_t Color; // RGBA
	float2 UV[MaxUV];
	int    BlendIndices[MaxBonesPerVertex];
	//float  BlendWeights[MaxBonesPerVertex];
	uint32_t BlendWeights; // Packed info byte values
	size_t BonesUsed = 0;

	float GetBlendWeight(size_t Index) const;
	void SetBlendWeight(size_t Index, float Weight);
};

struct CVertexFormat
{
	size_t NormalCount = 0;
	size_t TangentCount = 0;
	size_t BitangentCount = 0;
	size_t ColorCount = 0;
	size_t UVCount = 0;
	size_t BonesPerVertex = 0;
};

struct CMeshGroup
{
	std::vector<CVertex> Vertices;
	std::vector<unsigned int> Indices;
	float3 AABBMin;
	float3 AABBMax;
};

struct CMeshAttrInfo
{
	std::string MeshID;
	std::vector<std::string> MaterialIDs; // Per group (submesh)
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
};

void ProcessGeometry(const std::vector<CVertex>& RawVertices, const std::vector<unsigned int>& RawIndices, std::vector<CVertex>& Vertices, std::vector<unsigned int>& Indices);
void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex);
bool WriteDEMMesh(const std::filesystem::path& DestPath, const std::map<std::string, CMeshGroup>& SubMeshes, const CVertexFormat& VertexFormat, size_t BoneCount, CThreadSafeLog& Log);
bool WriteDEMSkin(const std::filesystem::path& DestPath, const std::vector<CBone>& Bones, CThreadSafeLog& Log);
