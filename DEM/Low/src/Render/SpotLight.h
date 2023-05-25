#pragma once
#include <Render/Light.h>

// Spot light source used for rendering

namespace Render
{
typedef std::unique_ptr<class CSpotLight> PSpotLight;

class CSpotLight : public CLight
{
	RTTI_CLASS_DECL(CSpotLight, CLight);

protected:

	// vector3 or acl::Vector4_32 _Direction;
	// position
	// range
	// cone angles
	// cached values

	//!!!can store data so that it is easily copied into the GPU buffer! aligned 16, with paddings.

public:

};

}
