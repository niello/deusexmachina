#pragma once
#ifndef __DEM_L1_RENDER_SM30_SHADER_METADATA_H__
#define __DEM_L1_RENDER_SM30_SHADER_METADATA_H__

#include <Render/ShaderMetadata.h>
#include <Render/D3D9/D3D9Fwd.h>

// Shader Model 3.0 (for Direct3D 9.0c) shader metadata

namespace IO
{
	class CStream;
}

namespace Render
{

enum ESM30ConstFlags
{
	SM30Const_ColumnMajor	= 0x01			// Only for matrix types
};

struct CSM30ShaderBufferMeta
{
	CStrID				Name;
	U32					SlotIndex;			// Pseudo-register
	CFixedArray<CRange>	Float4;
	CFixedArray<CRange>	Int4;
	CFixedArray<CRange>	Bool;
	HHandle				Handle;
};

struct CSM30ShaderConstMeta
{
	HHandle				BufferHandle;
	CStrID				Name;
	ESM30RegisterSet	RegSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;
	U8					Flags;				// See ESM30ConstFlags
	HHandle				Handle;
};

struct CSM30ShaderRsrcMeta
{
	CStrID				Name;
	U32					Register;
	HHandle				Handle;
};

struct CSM30ShaderSamplerMeta
{
	CStrID				Name;
	ESM30SamplerType	Type;
	U32					RegisterStart;
	U32					RegisterCount;
	HHandle				Handle;
};

class CSM30ShaderMetadata: public IShaderMetadata
{
private:

	CFixedArray<CSM30ShaderBufferMeta>	Buffers;
	CFixedArray<CSM30ShaderConstMeta>	Consts;
	CFixedArray<CSM30ShaderRsrcMeta>	Resources;
	CFixedArray<CSM30ShaderSamplerMeta>	Samplers;

public:

	virtual ~CSM30ShaderMetadata() { Clear(); }

	bool						Load(IO::CStream& Stream);
	void						Clear();

	virtual EGPUFeatureLevel	GetMinFeatureLevel() const { return GPU_Level_D3D9_3; }
	virtual HConst				GetConstHandle(CStrID ID) const;
	virtual HConstBuffer		GetConstBufferHandle(CStrID ID) const;
	virtual HResource			GetResourceHandle(CStrID ID) const;
	virtual HSampler			GetSamplerHandle(CStrID ID) const;
	virtual bool				GetConstDesc(CStrID ID, CShaderConstDesc& Out) const;
};

}

#endif
