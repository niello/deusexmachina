#pragma once
#ifndef __DEM_L1_RENDER_D3D9_FWD_H__
#define __DEM_L1_RENDER_D3D9_FWD_H__

#include <Data/StringID.h>
#include <Data/FixedArray.h>

// Direct3D9 render implementation forward declaration

namespace Render
{

// Don't change values
enum ED3D9ShaderRegisterSet
{
	Reg_Float4	= 0,
	Reg_Int4	= 1,
	Reg_Bool	= 2
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
	CStrID					Name;
	ED3D9ShaderRegisterSet	RegSet;
	UPTR					Offset;
	UPTR					Size;
	HHandle					Handle;
	HHandle					BufferHandle;
};

struct CD3D9ShaderRsrcMeta
{
	CStrID				SamplerName;
	CStrID				TextureName;
	UPTR				Register;
	HHandle				Handle;
};

}

#endif
