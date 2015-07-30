#include "SamplerDesc.h"

namespace Render
{

void CSamplerDesc::SetDefaults()
{
	AddressU = TexAddr_Wrap; // In D3D11 clamp
	AddressV = TexAddr_Wrap;
	AddressW = TexAddr_Wrap;
	MipMapLODBias = 0.f;
	BorderColorRGBA[0] = 0.f; // In D3D11 white (all 1.f)
	BorderColorRGBA[1] = 0.f;
	BorderColorRGBA[2] = 0.f;
	BorderColorRGBA[3] = 0.f;
	FinestMipMapLOD = 0;
	CoarsestMipMapLOD = FLT_MAX;
	MaxAnisotropy = 1;
	CmpFunc = Cmp_Never;
//Filter	D3D11_FILTER_MIN_MAG_MIP_LINEAR
//D3DTEXTUREFILTERTYPE D3D9_MAGFILTER, default D3DTEXF_POINT
//D3DTEXTUREFILTERTYPE D3D9_MINFILTER, default D3DTEXF_POINT
//D3DTEXTUREFILTERTYPE D3D9_MIPFILTER, default D3DTEXF_NONE
}
//---------------------------------------------------------------------

}