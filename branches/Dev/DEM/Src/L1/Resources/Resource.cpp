#include "Resource.h"

#include <Resources/ResourceServer.h>

namespace Resources
{
ImplementRTTI(Resources::CResource, Core::CRefCounted);

CResource::~CResource()
{
	RsrcSrv->ReleaseResource(this);
}
//---------------------------------------------------------------------

}