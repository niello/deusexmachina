#pragma once
#ifndef __DEM_L1_SAMPLER_DESC_H__
#define __DEM_L1_SAMPLER_DESC_H__

#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// API-independent description of a sampler state. It can be
// serialized, deserialized and used for creation of CSampler objects.

namespace Render
{

struct CSamplerDesc
{
	ETexAddressMode	AddressU;
	ETexAddressMode	AddressV;
	ETexAddressMode	AddressW;
	ETexFilter		Filter;
	float			MipMapLODBias;
	float			BorderColorRGBA[4];
	float			FinestMipMapLOD;
	float			CoarsestMipMapLOD;
	unsigned int	MaxAnisotropy;
	ECmpFunc		CmpFunc;

	void SetDefaults();
};

}

#endif
