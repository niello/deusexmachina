#include "CDLODData.h"

namespace Render
{
__ImplementClassNoFactory(Render::CCDLODData, Resources::CResourceObject);

CCDLODData::CCDLODData() {}

CCDLODData::~CCDLODData()
{
	if (pMinMaxData) n_free(pMinMaxData);
}
//---------------------------------------------------------------------

}