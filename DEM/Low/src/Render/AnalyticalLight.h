#pragma once
#include <Render/Light.h>

// Analytical light source used for rendering. Uses math for calculating rediance.
// Directional, point, spot, rect lights etc are analytical. Other type is IBL.

namespace Render
{
typedef std::unique_ptr<class CAnalyticalLight> PAnalyticalLight;

class CAnalyticalLight : public CLight
{
	RTTI_CLASS_DECL(CAnalyticalLight, CLight);

public:

	CAnalyticalLight(ELightType Type)
	{
		n_assert_dbg(Type != ELightType::IBL);
		GPUData.Type = Type;
	}
};

}
