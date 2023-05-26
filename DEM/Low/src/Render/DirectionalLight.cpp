#include "DirectionalLight.h"
#include <Math/Matrix44.h>

namespace Render
{

void CDirectionalLight::UpdateTransform(const matrix44& Tfm)
{
	_Direction = acl::vector_set(-Tfm.AxisZ().x, -Tfm.AxisZ().y, -Tfm.AxisZ().z);
}
//---------------------------------------------------------------------

}
