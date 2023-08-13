#pragma once
#include <Render/Light.h>
#include <Math/Vector3.h>
#include <acl/math/vector4_32.h>

// Spot light source used for rendering

namespace Render
{
typedef std::unique_ptr<class CSpotLight> PSpotLight;

class CSpotLight : public CLight
{
	RTTI_CLASS_DECL(CSpotLight, CLight);

protected:

	acl::Vector4_32 _Direction; //!!!use w for something! each light params can be just passed to array of float4 registers. or can use common structure for structured buffer.

	// position
	// range
	// cone angles
	// cached values

	//!!!can store data so that it is easily copied into the GPU buffer! aligned 16, with paddings.

public:

	void SetDirection(const vector3& Dir);
};

}
