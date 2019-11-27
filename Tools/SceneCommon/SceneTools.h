#pragma once
#include <Render/RenderEnums.h>
#include <vector>
#include <map>
#include <filesystem>

// Common tools for scene hierarchy and resources processing.
// Used by converters from different common formats (FBX, glTF) to DEM.

// According to the core glTF 2.0 spec:
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
constexpr size_t MaxUV = 2;
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

	float2() : x(0.f), y(0.f) {}
	float2(float x_, float y_) : x(x_), y(y_) {}
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

	float3() : x(0.f), y(0.f), z(0.f) {}
	float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
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

	float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
	float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
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

struct CMeshData
{
	std::vector<CVertex> Vertices;
	std::vector<unsigned int> Indices;
	float3 AABBMin;
	float3 AABBMax;
};

void ProcessGeometry(const std::vector<CVertex>& RawVertices, const std::vector<unsigned int>& RawIndices, std::vector<CVertex>& Vertices, std::vector<unsigned int>& Indices);
void WriteVertexComponent(std::ostream& Stream, EVertexComponentSemantic Semantic, EVertexComponentFormat Format, uint8_t Index, uint8_t StreamIndex);
bool WriteDEMMesh(const std::filesystem::path& DestPath, const std::map<std::string, CMeshData>& SubMeshes, const CVertexFormat& VertexFormat, size_t BoneCount, CThreadSafeLog& Log);
