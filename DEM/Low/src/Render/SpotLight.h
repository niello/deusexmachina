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

	vector3 _Position;
	vector3 _Direction;

public:

	float Range = 0.f;
	float CosHalfInner = 0.f;
	float CosHalfOuter = 0.f;

	virtual void FillGPUInfo(CGPULightInfo& Out) const override;

	void SetPosition(const vector3& Pos);
	void SetDirection(const vector3& Dir);
};

}
