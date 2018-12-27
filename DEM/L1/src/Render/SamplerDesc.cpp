#include "SamplerDesc.h"

namespace Render
{

void CSamplerDesc::SetDefaults()
{
	AddressU = TexAddr_Wrap;				// In D3D11 clamp
	AddressV = TexAddr_Wrap;
	AddressW = TexAddr_Wrap;
	Filter = TexFilter_MinMagMip_Linear;	// In D3D9 point, point, none (all point)
	MipMapLODBias = 0.f;
	BorderColorRGBA[0] = 0.f;				// In D3D11 white (all 1.f)
	BorderColorRGBA[1] = 0.f;
	BorderColorRGBA[2] = 0.f;
	BorderColorRGBA[3] = 0.f;
	FinestMipMapLOD = 0;
	CoarsestMipMapLOD = FLT_MAX;
	MaxAnisotropy = 1;
	CmpFunc = Cmp_Never;
}
//---------------------------------------------------------------------

}