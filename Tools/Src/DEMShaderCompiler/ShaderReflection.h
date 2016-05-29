#pragma once
#ifndef __DEM_TOOLS_SHADER_REFLECTION_H__
#define __DEM_TOOLS_SHADER_REFLECTION_H__

#include <Data/String.h>
#include <Data/Array.h>

// Data and functions for shader metadata manipulation in both SM3.0 and USM

namespace IO
{
	class CBinaryReader;
	class CBinaryWriter;
};

// Don't change values, they are saved to file
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

// Don't change values, they are saved to file
enum ESM30RegisterSet
{
	RS_Bool		= 0,
	RS_Int4		= 1,
	RS_Float4	= 2
};

struct CSM30ShaderBufferMeta
{
	CString			Name;
	CArray<UPTR>	UsedFloat4;
	CArray<UPTR>	UsedInt4;
	CArray<UPTR>	UsedBool;
};

// Arrays and single-type structures are supported
struct CSM30ShaderConstMeta
{
	CString				Name;
	U32					BufferIndex;
	ESM30RegisterSet	RegisterSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;

	U32					RegisterCount; // Cache, not saved

	bool operator ==(const CSM30ShaderConstMeta& Other) const { return RegisterSet == Other.RegisterSet && RegisterStart == Other.RegisterStart && ElementRegisterCount == Other.ElementRegisterCount && ElementCount == Other.ElementCount; }
	bool operator !=(const CSM30ShaderConstMeta& Other) const { return RegisterSet != Other.RegisterSet || RegisterStart != Other.RegisterStart || ElementRegisterCount != Other.ElementRegisterCount || ElementCount != Other.ElementCount; }
};

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
struct CSM30ShaderRsrcMeta
{
	CString	Name;
	U32		Register;

	bool operator ==(const CSM30ShaderRsrcMeta& Other) const { return Register == Other.Register; }
	bool operator !=(const CSM30ShaderRsrcMeta& Other) const { return Register != Other.Register; }
};

// Arrays supported with arbitrarily assigned textures
struct CSM30ShaderSamplerMeta
{
	CString				Name;
	ESM30SamplerType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	bool operator ==(const CSM30ShaderSamplerMeta& Other) const { return Type == Other.Type && RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CSM30ShaderSamplerMeta& Other) const { return Type != Other.Type || RegisterStart != Other.RegisterStart || RegisterCount != Other.RegisterCount; }
};

struct CSM30ShaderMeta
{
	CArray<CSM30ShaderBufferMeta>	Buffers;
	CArray<CSM30ShaderConstMeta>	Consts;
	CArray<CSM30ShaderRsrcMeta>		Resources;
	CArray<CSM30ShaderSamplerMeta>	Samplers;
};

// Don't change values, they are saved to file
enum EUSMBufferTypeMask
{
	USMBuffer_Texture		= (1 << 30),
	USMBuffer_Structured	= (2 << 30),

	USMBuffer_RegisterMask = ~(USMBuffer_Texture | USMBuffer_Structured)
};

// Don't change values, they are saved to file
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change values, they are saved to file
enum EUSMResourceType
{
	USMRsrc_Texture1D			= 0,
	USMRsrc_Texture1DArray,
	USMRsrc_Texture2D,
	USMRsrc_Texture2DArray,
	USMRsrc_Texture2DMS,
	USMRsrc_Texture2DMSArray,
	USMRsrc_Texture3D,
	USMRsrc_TextureCUBE,
	USMRsrc_TextureCUBEArray,

	USMRsrc_Unknown
};

struct CUSMShaderBufferMeta
{
	CString	Name;
	U32		Register;
	U32		Size;		// For structured buffers - StructureByteStride

	bool operator ==(const CUSMShaderBufferMeta& Other) const { return Register == Other.Register && Size == Other.Size; }
	bool operator !=(const CUSMShaderBufferMeta& Other) const { return Register != Other.Register || Size != Other.Size; }
};

// Supports SM4.0 and higher
// Arrays and mixed-type structs supported
struct CUSMShaderConstMeta
{
	CString			Name;
	U32				BufferIndex;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;

	bool operator ==(const CUSMShaderConstMeta& Other) const { return Type == Other.Type && Offset == Other.Offset && ElementSize == Other.ElementSize && ElementCount == Other.ElementCount; }
	bool operator !=(const CUSMShaderConstMeta& Other) const { return Type != Other.Type || Offset != Other.Offset || ElementSize != Other.ElementSize || ElementCount != Other.ElementCount; }
};

// Supports SM4.0 and higher
struct CUSMShaderRsrcMeta
{
	CString				Name;
	EUSMResourceType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	bool operator ==(const CUSMShaderRsrcMeta& Other) const { return Type == Other.Type && RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CUSMShaderRsrcMeta& Other) const { return Type != Other.Type && RegisterStart != Other.RegisterStart && RegisterCount != Other.RegisterCount; }
};

// Supports SM4.0 and higher
struct CUSMShaderSamplerMeta
{
	CString	Name;
	U32		RegisterStart;
	U32		RegisterCount;

	bool operator ==(const CUSMShaderSamplerMeta& Other) const { return RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CUSMShaderSamplerMeta& Other) const { return RegisterStart != Other.RegisterStart && RegisterCount != Other.RegisterCount; }
};

struct CUSMShaderMeta
{
	U32								MinFeatureLevel;
	U64								RequiresFlags;
	CArray<CUSMShaderBufferMeta>	Buffers;
	CArray<CUSMShaderConstMeta>	Consts;
	CArray<CUSMShaderRsrcMeta>		Resources;
	CArray<CUSMShaderSamplerMeta>	Samplers;
};

void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName = NULL);
void ReadRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryReader& R);
bool D3D9CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CSM30ShaderMeta& Out);
bool USMCollectShaderMetadata(const void* pData, UPTR Size, CUSMShaderMeta& Out);
bool D3D9SaveShaderMetadata(IO::CBinaryWriter& W, const CSM30ShaderMeta& Meta);
bool USMSaveShaderMetadata(IO::CBinaryWriter& W, const CUSMShaderMeta& Meta);
bool D3D9LoadShaderMetadata(IO::CBinaryReader& R, CSM30ShaderMeta& Meta);
bool USMLoadShaderMetadata(IO::CBinaryReader& R, CUSMShaderMeta& Meta);

#endif
