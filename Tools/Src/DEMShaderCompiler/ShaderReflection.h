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

enum EShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

///////////////////////////////////////////////////////////////////////
// Legacy Shader Model 3.0 (SM3.0)
///////////////////////////////////////////////////////////////////////

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

struct CSM30BufferMeta
{
	CString			Name;
	U32				SlotIndex;	// Pseudo-register, two buffers at the same slot will conflict
	CArray<UPTR>	UsedFloat4;
	CArray<UPTR>	UsedInt4;
	CArray<UPTR>	UsedBool;

	//bool operator ==(const CSM30BufferMeta& Other) const { return SlotIndex == Other.SlotIndex; }
	//bool operator !=(const CSM30BufferMeta& Other) const { return SlotIndex != Other.SlotIndex; }
};

struct CSM30StructMemberMeta
{
	CString			Name;
	U32				StructIndex;
	U32				RegisterOffset;
	U32				ElementRegisterCount;
	U32				ElementCount;
	U8				Flags;					// See EShaderConstFlags
	//???store register set and support mixed structs?
};

struct CSM30StructMeta
{
	//CString						Name;
	CArray<CSM30StructMemberMeta>	Members;
};

// Arrays and single-type structures are supported
struct CSM30ConstMeta
{
	CString				Name;
	U32					BufferIndex;
	U32					StructIndex;
	ESM30RegisterSet	RegisterSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;
	U8					Flags;					// See EShaderConstFlags

	U32					RegisterCount;			// Cache, not saved

	bool operator ==(const CSM30ConstMeta& Other) const { return RegisterSet == Other.RegisterSet && RegisterStart == Other.RegisterStart && ElementRegisterCount == Other.ElementRegisterCount && ElementCount == Other.ElementCount && Flags == Other.Flags; }
	bool operator !=(const CSM30ConstMeta& Other) const { return RegisterSet != Other.RegisterSet || RegisterStart != Other.RegisterStart || ElementRegisterCount != Other.ElementRegisterCount || ElementCount != Other.ElementCount || Flags != Other.Flags; }
};

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
struct CSM30RsrcMeta
{
	CString	Name;
	U32		Register;

	bool operator ==(const CSM30RsrcMeta& Other) const { return Register == Other.Register; }
	bool operator !=(const CSM30RsrcMeta& Other) const { return Register != Other.Register; }
};

// Arrays supported with arbitrarily assigned textures
struct CSM30SamplerMeta
{
	CString				Name;
	ESM30SamplerType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	bool operator ==(const CSM30SamplerMeta& Other) const { return Type == Other.Type && RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CSM30SamplerMeta& Other) const { return Type != Other.Type || RegisterStart != Other.RegisterStart || RegisterCount != Other.RegisterCount; }
};

struct CSM30ShaderMeta
{
	CArray<CSM30BufferMeta>		Buffers;
	CArray<CSM30StructMeta>		Structs;
	CArray<CSM30ConstMeta>		Consts;
	CArray<CSM30RsrcMeta>		Resources;
	CArray<CSM30SamplerMeta>	Samplers;
};

///////////////////////////////////////////////////////////////////////
// Universal Shader Model (SM4.0 and higher)
///////////////////////////////////////////////////////////////////////

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

struct CUSMBufferMeta
{
	CString	Name;
	U32		Register;
	U32		Size;		// For structured buffers - StructureByteStride

	bool operator ==(const CUSMBufferMeta& Other) const { return Register == Other.Register && Size == Other.Size; }
	bool operator !=(const CUSMBufferMeta& Other) const { return Register != Other.Register || Size != Other.Size; }
};

struct CUSMStructMemberMeta
{
	CString			Name;
	U32				StructIndex;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Flags;			// See EShaderConstFlags
};

struct CUSMStructMeta
{
	//CString						Name;
	CArray<CUSMStructMemberMeta>	Members;
};

// Arrays and mixed-type structs supported
struct CUSMConstMeta
{
	CString			Name;
	U32				BufferIndex;
	U32				StructIndex;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Flags;			// See EShaderConstFlags

	bool operator ==(const CUSMConstMeta& Other) const { return Type == Other.Type && Offset == Other.Offset && ElementSize == Other.ElementSize && ElementCount == Other.ElementCount; }
	bool operator !=(const CUSMConstMeta& Other) const { return Type != Other.Type || Offset != Other.Offset || ElementSize != Other.ElementSize || ElementCount != Other.ElementCount; }
};

struct CUSMRsrcMeta
{
	CString				Name;
	EUSMResourceType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	bool operator ==(const CUSMRsrcMeta& Other) const { return Type == Other.Type && RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CUSMRsrcMeta& Other) const { return Type != Other.Type && RegisterStart != Other.RegisterStart && RegisterCount != Other.RegisterCount; }
};

struct CUSMSamplerMeta
{
	CString	Name;
	U32		RegisterStart;
	U32		RegisterCount;

	bool operator ==(const CUSMSamplerMeta& Other) const { return RegisterStart == Other.RegisterStart && RegisterCount == Other.RegisterCount; }
	bool operator !=(const CUSMSamplerMeta& Other) const { return RegisterStart != Other.RegisterStart && RegisterCount != Other.RegisterCount; }
};

struct CUSMShaderMeta
{
	U32						MinFeatureLevel;
	U64						RequiresFlags;
	CArray<CUSMBufferMeta>	Buffers;
	CArray<CUSMStructMeta>	Structs;
	CArray<CUSMConstMeta>	Consts;
	CArray<CUSMRsrcMeta>	Resources;
	CArray<CUSMSamplerMeta>	Samplers;
};

class CDEMD3DInclude;

bool SM30CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CDEMD3DInclude& IncludeHandler, CSM30ShaderMeta& Out);
bool SM30SaveShaderMetadata(IO::CBinaryWriter& W, const CSM30ShaderMeta& Meta);
bool SM30LoadShaderMetadata(IO::CBinaryReader& R, CSM30ShaderMeta& Meta);

bool USMCollectShaderMetadata(const void* pData, UPTR Size, CUSMShaderMeta& Out);
bool USMSaveShaderMetadata(IO::CBinaryWriter& W, const CUSMShaderMeta& Meta);
bool USMLoadShaderMetadata(IO::CBinaryReader& R, CUSMShaderMeta& Meta);

#endif
