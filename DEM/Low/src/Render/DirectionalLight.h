#pragma once
#include <Render/Light.h>

// Directional light source used for rendering. Always global.

namespace Render
{
typedef std::unique_ptr<class CDirectionalLight> PDirectionalLight;

class CDirectionalLight : public CLight
{
	RTTI_CLASS_DECL(CDirectionalLight, CLight);

protected:

	vector3 _Direction;

public:

	virtual void FillGPUInfo(CGPULightInfo& Out) const override;

	void SetDirection(const vector3& Dir);
};

}
