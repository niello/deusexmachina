#include "TextureData.h"
#include <Data/Buffer.h>

namespace Render
{

CTextureData::CTextureData() = default;
CTextureData::~CTextureData() = default;

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
