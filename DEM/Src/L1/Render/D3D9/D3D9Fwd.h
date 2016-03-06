#pragma once
#ifndef __DEM_L1_RENDER_D3D9_FWD_H__
#define __DEM_L1_RENDER_D3D9_FWD_H__

#include <Data/StringID.h>
#include <Data/FixedArray.h>

// Direct3D9 render implementation forward declaration

namespace Render
{

// Don't change values
enum ESM30RegisterSet
{
	Reg_Bool			= 0,
	Reg_Int4			= 1,
	Reg_Float4			= 2
};

// Don't change values
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

struct CD3D9ShaderBufferMeta
{
	CStrID				Name;
	CFixedArray<CRange>	Float4;
	CFixedArray<CRange>	Int4;
	CFixedArray<CRange>	Bool;
	UPTR				SlotIndex;	// Pseudoregister, always equal to an index in a shader buffers metadata array
	HHandle				Handle;
};

struct CD3D9ShaderConstMeta
{
	HHandle				BufferHandle;
	CStrID				Name;
	ESM30RegisterSet	RegSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;
	HHandle				Handle;
};

struct CD3D9ShaderRsrcMeta
{
	CStrID				Name;
	U32					Register;
	HHandle				Handle;
};

struct CD3D9ShaderSamplerMeta
{
	CStrID				Name;
	ESM30SamplerType	Type;
	U32					RegisterStart;
	U32					RegisterCount;
	HHandle				Handle;
};

}

#endif
