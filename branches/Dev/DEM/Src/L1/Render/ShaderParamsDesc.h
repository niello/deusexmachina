#pragma once
#ifndef __DEM_L1_RENDER_SHADER_PARAMS_DESC_H__
#define __DEM_L1_RENDER_SHADER_PARAMS_DESC_H__

#include <Data/RefCounted.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>
#include <Render/RenderFwd.h>

// Description of a shader technique parameters. 

namespace Render
{

struct CShaderConstantDesc
{
	HShaderParam Handle;
	// Buffer index, Offset in buffer
	// Type (float, int, bool, ?texture? etc)
	// Size (when applicable)
	//???bitmask of shader types where this var is used? VS, PS etc
	//each buffer also can store that mask if needed (DX11) - useful for binding CBs
	//simple detection of unused variables
};

struct CShaderConstBufferDesc
{
	HShaderParam	Handle;
	DWORD			Register;
	DWORD			ByteSize;
	Data::CFlags	Flags;		// Static/Dynamic, Frequency(or separate?)
};

struct CShaderResourceDesc
{
	HShaderParam	Handle;
	DWORD			Register;
};

struct CShaderSamplerDesc
{
	HShaderParam	Handle;
	DWORD			Register;
};

class CShaderParamsDesc: public Data::CRefCounted
{
protected:

	CDict<CStrID, CShaderConstantDesc> ConstDescs;
	//CDict<CStrID, CShaderBufferDesc> BufferDesc; // DX11, DX9 uses a single buffer without desc
	//var handle can be a pointer to CShaderConstantDesc for DX9, to get type, or type can be packed inside

public:

	const CShaderConstantDesc*	GetConstantDesc(CStrID Name) const;
	//!!!texture/buffer/shader-resource desc, sampler state desc!
	//???or one desc for all and type inside? CShaderParamDesc
};

typedef Ptr<CShaderMetadata> PShaderMetadata;

inline const CShaderConstantDesc* CShaderMetadata::GetConstantDesc(CStrID Name) const
{
	int Idx = ConstDescs.FindIndex(Name);
	return Idx == INVALID_INDEX ? NULL : &ConstDescs.ValueAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
