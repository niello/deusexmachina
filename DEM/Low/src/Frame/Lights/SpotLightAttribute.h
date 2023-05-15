#pragma once
#include <Frame/LightAttribute.h>

// Spot light in a scene

namespace Frame
{

class CSpotLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	U32   _Color = 0xffffffff;
	float _Intensity = 1.f;
	float _Range = 1.f;
	float _ConeInner = n_deg2rad(30.f); // In radians, full angle (not half), Theta
	float _ConeOuter = n_deg2rad(45.f); // In radians, full angle (not half), Phi

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight(CGraphicsResourceManager& ResMgr) const override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
};

typedef Ptr<CSpotLightAttribute> PSpotLightAttribute;

}
