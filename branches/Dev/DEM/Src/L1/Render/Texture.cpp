#include "Texture.h"

namespace Render
{

DWORD CTexture::GetPixelCount(bool IncludeSubsequentMips) const
{
	DWORD BaseCount;
	switch (Desc.Type)
	{
		case Texture_2D:	BaseCount = Desc.Width * Desc.Height;
		case Texture_3D:	BaseCount = Desc.Width * Desc.Height * Desc.Depth;
		case Texture_Cube:	BaseCount = Desc.Width * Desc.Height * 6;
		default:			return 0;
	}

	if (!IncludeSubsequentMips) return BaseCount;

	DWORD DivShift = ((Desc.Type == Texture_3D) ? 3 : 2); // Divide by 8 or 4
	DWORD Accum = BaseCount;
	for (DWORD i = 1; i < Desc.MipLevels; ++i)
	{
		BaseCount >>= DivShift;
		Accum += BaseCount;
	}

	return Accum;
}
//---------------------------------------------------------------------

}