#include "CDLODData.h"

namespace Render
{
__ImplementClassNoFactory(Render::CCDLODData, Resources::CResourceObject);

CCDLODData::CCDLODData(): pMinMaxData(NULL)
{
}
//---------------------------------------------------------------------

CCDLODData::~CCDLODData()
{
	if (pMinMaxData) n_free(pMinMaxData);
}
//---------------------------------------------------------------------

}