#pragma once
#ifndef __DEM_L1_RENDER_USM_SHADER_METADATA_H__
#define __DEM_L1_RENDER_USM_SHADER_METADATA_H__

#include <Render/ShaderMetadata.h>
#include <Render/D3D9/D3D9Fwd.h>

// Universal Shader Model (4.0 and higher, for Direct3D 10 and higher) shader metadata

namespace IO
{
	class CStream;
}

namespace Render
{

// Don't change values
enum EUSMBufferType
{
	USMBuffer_Constant		= 0,
	USMBuffer_Texture		= 1,
	USMBuffer_Structured	= 2
};

// Don't change values
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change values
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
	CStrID			Name;
	EUSMBufferType	Type;
	U32				Register;
	U32				Size;			// Element size for structured buffers
	HHandle			Handle;
};

struct CUSMStructMemberMeta
{
	CStrID			Name;
	HHandle			StructHandle;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Columns;
	U8				Rows;
	U8				Flags;			// See EShaderConstFlags
};

struct CUSMStructMeta
{
	HHandle								Handle;
	CFixedArray<CUSMStructMemberMeta>	Members;
};

struct CUSMConstMeta
{
	CStrID			Name;
	HHandle			BufferHandle;
	HHandle			StructHandle;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Columns;
	U8				Rows;
	U8				Flags;			// See EShaderConstFlags
	HHandle			Handle;
	PShaderConstant	ConstObject;
};

struct CUSMResourceMeta
{
	CStrID				Name;
	EUSMResourceType	Type;
	U32					RegisterStart;
	U32					RegisterCount;
	HHandle				Handle;
};

struct CUSMSamplerMeta
{
	CStrID		Name;
	U32			RegisterStart;
	U32			RegisterCount;
	HHandle		Handle;
};

class CUSMShaderMetadata: public IShaderMetadata
{
private:

	//!!!must be sorted by name! //???sort in tool?
	EGPUFeatureLevel				MinFeatureLevel;
	U64								RequiresFlags;	//!!!add getter!
	CFixedArray<CUSMConstMeta>		Consts;
	CFixedArray<CUSMStructMeta>		Structs;
	CFixedArray<CUSMBufferMeta>		Buffers;
	CFixedArray<CUSMResourceMeta>	Resources;
	CFixedArray<CUSMSamplerMeta>	Samplers;

public:

	virtual ~CUSMShaderMetadata() { Clear(); }

	bool						Load(IO::CStream& Stream);
	void						Clear();

	virtual EGPUFeatureLevel	GetMinFeatureLevel() const { return MinFeatureLevel; }
	virtual HConst				GetConstHandle(CStrID ID) const;
	virtual HConstBuffer		GetConstBufferHandle(CStrID ID) const;
	virtual HResource			GetResourceHandle(CStrID ID) const;
	virtual HSampler			GetSamplerHandle(CStrID ID) const;
	virtual PShaderConstant		GetConstant(HConst hConst) const;
};

}

#endif
