#include "Texture.h"

namespace Render
{
__ImplementClassNoFactory(Render::CTexture, Resources::CResourceObject);

UPTR CTexture::GetPixelCount(bool IncludeSubsequentMips) const
{
	UPTR BaseCount;
	switch (Desc.Type)
	{
		case Texture_2D:	BaseCount = Desc.Width * Desc.Height;
		case Texture_3D:	BaseCount = Desc.Width * Desc.Height * Desc.Depth;
		case Texture_Cube:	BaseCount = Desc.Width * Desc.Height * 6;
		default:			return 0;
	}

	if (!BaseCount || !IncludeSubsequentMips) return BaseCount;

	const UPTR DivShift = ((Desc.Type == Texture_3D) ? 3 : 2); // Divide by 8 or 4
	UPTR Accum = BaseCount;
	for (UPTR i = 1; i < Desc.MipLevels; ++i)
	{
		BaseCount >>= DivShift;
		Accum += BaseCount ? BaseCount : 1; // Each mip level takes at least 1x1 pixel
	}

	return Accum;
}
//---------------------------------------------------------------------

}