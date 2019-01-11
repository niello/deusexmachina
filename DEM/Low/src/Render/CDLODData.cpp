#include "CDLODData.h"
#include <Render/Texture.h>

namespace Render
{
__ImplementClassNoFactory(Render::CCDLODData, Resources::CResourceObject);

CCDLODData::CCDLODData()
{
}
//---------------------------------------------------------------------

CCDLODData::~CCDLODData()
{
	if (pMinMaxData) n_free(pMinMaxData);
}
//---------------------------------------------------------------------

bool CCDLODData::IsResourceValid() const
{
	return HeightMap.IsValidPtr();
}
//---------------------------------------------------------------------

Render::CTexture* CCDLODData::GetHeightMap() const
{
	return HeightMap.Get();
}
//---------------------------------------------------------------------

Render::CTexture* CCDLODData::GetNormalMap() const
{ 
	return NormalMap.Get();
}
//---------------------------------------------------------------------

}