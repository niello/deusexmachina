#pragma once
#include <Frame/LightAttribute.h>

// Spot light in a scene

namespace Frame
{

class CSpotLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	U32   _Color = Render::Color_White;
	float _Intensity = 1.f;
	float _Range = 1.f;
	float _BoundingSphereOffsetAlongDir = 0.f;
	float _BoundingSphereRadius = 0.f;
	float _ConeInner = n_deg2rad(30.f); // In radians, full angle (not half), Theta
	float _ConeOuter = n_deg2rad(45.f); // In radians, full angle (not half), Phi
	float _SinHalfOuter = 0.f;
	float _CosHalfOuter = 0.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(acl::Vector4_32 SphereCenter, float SphereRadius) const override;
	virtual bool                  DoesEmitAnyEnergy() const override { return (_Color & 0x00ffffff) && _Intensity > 0.f; }
};

typedef Ptr<CSpotLightAttribute> PSpotLightAttribute;

}
