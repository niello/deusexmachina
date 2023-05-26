#pragma once
#include <Render/Light.h>

// Point light source used for rendering

namespace Render
{
typedef std::unique_ptr<class CPointLight> PPointLight;

class CPointLight : public CLight
{
	RTTI_CLASS_DECL(CPointLight, CLight);

protected:

	// vector3 or acl::Vector4_32 _Direction;
	// position
	// range
	// cached values

	//!!!can store data so that it is easily copied into the GPU buffer! aligned 16, with paddings.

public:

	virtual void UpdateTransform(const matrix44& Tfm) override;
};

}
