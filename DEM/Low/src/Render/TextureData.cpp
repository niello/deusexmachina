#include "TextureData.h"
#include <Data/Buffer.h>

namespace Render
{
RTTI_CLASS_IMPL(Render::CTextureData, Resources::CResourceObject);

CTextureData::CTextureData() {}
CTextureData::~CTextureData() {}

bool CTextureData::UseBuffer()
{
	if (!Data) FAIL;
	++BufferUseCounter;
	OK;
}
//---------------------------------------------------------------------

void CTextureData::ReleaseBuffer()
{
	--BufferUseCounter;
	if (!BufferUseCounter) Data.reset();
}
//---------------------------------------------------------------------

}