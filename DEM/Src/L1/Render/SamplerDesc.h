#pragma once
#ifndef __DEM_L1_RENDER_STATE_DESC_H__
#define __DEM_L1_RENDER_STATE_DESC_H__

#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// API-independent description of a sampler state. It can be
// serialized, deserialized and used for creation of CSampler objects.

namespace Render
{

struct CSamplerDesc
{
	ETexAddressMode	AddressU; // default wrap
	ETexAddressMode	AddressV;
	ETexAddressMode	AddressW;
	float			MipMapLODBias; // default 0 //*(DWORD*)&fMipMapLODBias
	float			BorderColorRGBA[4]; // default all zero
	float			FinestMipMapLOD;
	float			CoarsestMipMapLOD;
	unsigned int	MaxAnisotropy; // min 1, for D3D9 max is D3DCAPS9.MaxAnisotropy, for D3D11 max is 16
	ECmpFunc		CmpFunc; // no D3D9 support
//D3D11_FILTER Filter;

	void SetDefaults();
};

}

#endif
