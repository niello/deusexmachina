#include "CDLODData.h"

namespace Render
{
RTTI_CLASS_IMPL(Render::CCDLODData, Resources::CResourceObject);

CCDLODData::CCDLODData() {}

CCDLODData::~CCDLODData()
{
	if (pMinMaxData) n_free(pMinMaxData);
}
//---------------------------------------------------------------------

}