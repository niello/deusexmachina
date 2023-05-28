#pragma once
#include <Render/Light.h>
#include <acl/math/vector4_32.h>

// Directional light source used for rendering. Always global.

namespace Render
{
typedef std::unique_ptr<class CDirectionalLight> PDirectionalLight;

class CDirectionalLight : public CLight
{
	RTTI_CLASS_DECL(CDirectionalLight, CLight);

protected:

	acl::Vector4_32 _Direction;

public:

	void SetDirection(const vector3& Dir);
};

}
