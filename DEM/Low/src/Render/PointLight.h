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

	vector3 _Position;

public:

	float Range = 0.f;

	virtual void FillGPUInfo(CGPULightInfo& Out) const override;

	void SetPosition(const vector3& Pos);
};

}
