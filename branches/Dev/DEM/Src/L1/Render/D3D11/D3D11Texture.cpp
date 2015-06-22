#include "D3D11Texture.h"

#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D11Texture, 'TEX1', Render::CTexture);

bool CD3D11Texture::Create(ID3D11Texture2D* pTexture)
{
	n_assert(false);
	OK;
}
//---------------------------------------------------------------------

}