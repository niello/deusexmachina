#include "TextureData.h"
#include <IO/Stream.h>

namespace Render
{
__ImplementClassNoFactory(Render::CTextureData, Resources::CResourceObject);

CTextureData::~CTextureData()
{
	if (Stream) Stream->Unmap();
	else SAFE_FREE(pData);
}
//---------------------------------------------------------------------

}