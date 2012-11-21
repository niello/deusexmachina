#include "PassOcclusion.h"

#include <scene/nsceneserver.h>

namespace Render
{

void CPassOcclusion::Render()
{
	nSceneServer::Instance()->DoOcclusionQuery();
}
//---------------------------------------------------------------------

}