#pragma once
#ifndef __DEM_TOOLS_SHADER_REFLECTION_H__
#define __DEM_TOOLS_SHADER_REFLECTION_H__

#include <Data/String.h>
#include <Data/Array.h>

#define SRV_BUFFER 0xf000 // Highest bit of CVar::Register, SRV if set, CB if not

namespace IO
{
	class CBinaryReader;
	class CBinaryWriter;
};

// Don't change values, they are saved to file
enum ERegisterSet
{
	RS_BOOL = 0,
	RS_INT4,
	RS_FLOAT4,
	RS_SAMPLER
};

struct CD3D9ShaderBufferMeta
{
	CString			Name;
	CArray<UPTR>	UsedFloat4;
	CArray<UPTR>	UsedInt4;
	CArray<UPTR>	UsedBool;
};

struct CD3D9ShaderConstMeta
{
	CString			Name;
	ERegisterSet	RegSet; //???or type?
	U32				BufferIndex;
	U32				Offset;
	U32				Size;
	//type, array size

	bool operator ==(const CD3D9ShaderConstMeta& Other) const { return RegSet == Other.RegSet && Offset == Other.Offset && Size == Other.Size; }
	bool operator !=(const CD3D9ShaderConstMeta& Other) const { return RegSet != Other.RegSet || Offset != Other.Offset || Size != Other.Size; }
};

struct CD3D9ShaderRsrcMeta
{
	CString	TextureName;
	CString	SamplerName;
	U32		Register;

	bool operator ==(const CD3D9ShaderRsrcMeta& Other) const { return Register == Other.Register; }
	bool operator !=(const CD3D9ShaderRsrcMeta& Other) const { return Register != Other.Register; }
};

// For textures and samplers separately
//struct CD3D9ShaderRsrcMeta
//{
//	CString	Name;
//	U32		Registers;	// Bit field
//};

enum ED3D11BufferTypeMask
{
	D3D11Buffer_Texture		= (1 << 30),
	D3D11Buffer_Structured	= (2 << 30)
};

struct CD3D11ShaderBufferMeta
{
	CString	Name;
	U32		Register;
	U32		ElementSize;
	U32		ElementCount;

	bool operator ==(const CD3D11ShaderBufferMeta& Other) const { return Register == Other.Register && ElementSize == Other.ElementSize && ElementCount == Other.ElementCount; }
	bool operator !=(const CD3D11ShaderBufferMeta& Other) const { return Register != Other.Register || ElementSize != Other.ElementSize || ElementCount != Other.ElementCount; }
};

struct CD3D11ShaderConstMeta
{
	CString	Name;
	U32		BufferIndex;
	U32		Offset;
	U32		Size;
	//type, array size

	bool operator ==(const CD3D11ShaderConstMeta& Other) const { return Offset == Other.Offset && Size == Other.Size; }
	bool operator !=(const CD3D11ShaderConstMeta& Other) const { return Offset != Other.Offset || Size != Other.Size; }
};

struct CD3D11ShaderRsrcMeta
{
	CString	Name;
	U32		Register;

	bool operator ==(const CD3D11ShaderRsrcMeta& Other) const { return Register == Other.Register; }
	bool operator !=(const CD3D11ShaderRsrcMeta& Other) const { return Register != Other.Register; }
};

struct CD3D9ShaderMeta
{
	CArray<CD3D9ShaderConstMeta>	Consts;
	CArray<CD3D9ShaderBufferMeta>	Buffers;
	CArray<CD3D9ShaderRsrcMeta>		Samplers;
};

struct CD3D11ShaderMeta
{
	//???min feature level?
	CArray<CD3D11ShaderConstMeta>	Consts;
	CArray<CD3D11ShaderBufferMeta>	Buffers;
	CArray<CD3D11ShaderRsrcMeta>	Resources;
	CArray<CD3D11ShaderRsrcMeta>	Samplers;
};

void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName);
void ReadRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryReader& R);
bool D3D9CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CD3D9ShaderMeta& Out);
bool D3D11CollectShaderMetadata(const void* pData, UPTR Size, CD3D11ShaderMeta& Out);
bool D3D9SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D9ShaderMeta& Meta);
bool D3D11SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D11ShaderMeta& Meta);
bool D3D9LoadShaderMetadata(IO::CBinaryReader& R, CD3D9ShaderMeta& Meta);
bool D3D11LoadShaderMetadata(IO::CBinaryReader& R, CD3D11ShaderMeta& Meta);

#endif
