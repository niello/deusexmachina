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

struct CSM30BufferMeta
{
	CStrID				Name;
	U32					SlotIndex;			// Pseudo-register
	CFixedArray<CRange>	Float4;
	CFixedArray<CRange>	Int4;
	CFixedArray<CRange>	Bool;
	HHandle				Handle;
};

struct CSM30StructMemberMeta
{
	CStrID			Name;
	HHandle			StructHandle;
	U32				RegisterOffset;
	U32				ElementRegisterCount;
	U32				ElementCount;
	U8				Columns;
	U8				Rows;
	U8				Flags;					// See EShaderConstFlags
	//???store register set and support mixed structs?
};

struct CSM30StructMeta
{
	HHandle								Handle;
	CFixedArray<CSM30StructMemberMeta>	Members;
};

struct CSM30ConstMeta
{
	CStrID				Name;
	HHandle				BufferHandle;
	HHandle				StructHandle;
	ESM30RegisterSet	RegSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;
	U8					Columns;
	U8					Rows;
	U8					Flags;				// See ESM30ConstFlags
	HHandle				Handle;
	PShaderConstant		ConstObject;
};

struct CSM30RsrcMeta
{
	CStrID				Name;
	U32					Register;
	HHandle				Handle;
};

struct CSM30SamplerMeta
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

	CFixedArray<CSM30BufferMeta>	Buffers;
	CFixedArray<CSM30StructMeta>	Structs;
	CFixedArray<CSM30ConstMeta>		Consts;
	CFixedArray<CSM30RsrcMeta>		Resources;
	CFixedArray<CSM30SamplerMeta>	Samplers;

public:

	CSM30ShaderMetadata();
	virtual ~CSM30ShaderMetadata();

	bool						Load(IO::CStream& Stream);
	void						Clear();

	virtual EGPUFeatureLevel	GetMinFeatureLevel() const { return GPU_Level_D3D9_3; }
	virtual HConst				GetConstHandle(CStrID ID) const;
	virtual HConstBuffer		GetConstBufferHandle(CStrID ID) const;
	virtual HResource			GetResourceHandle(CStrID ID) const;
	virtual HSampler			GetSamplerHandle(CStrID ID) const;
	virtual PShaderConstant		GetConstant(HConst hConst) const;
};

}

#endif
