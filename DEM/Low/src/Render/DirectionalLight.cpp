#include "DirectionalLight.h"
#include <Math/Matrix44.h>

namespace Render
{

void CDirectionalLight::SetDirection(const vector3& Dir)
{
	_Direction = acl::vector_set(Dir.x, Dir.y, Dir.z);
}
//---------------------------------------------------------------------

}
