#pragma once
#include <Frame/LightAttribute.h>
#include <Math/CameraMath.h>

// Spot light in a scene

namespace Frame
{

class CSpotLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	mutable Math::CSIMDFrustum _WorldFrustum;                // Check validity by _WorldBoundsCacheVersion
	mutable U32                _WorldBoundsCacheVersion = 0; // Matches _pNode->TransformVersion

	U32   _Color = Render::Color_White;
	float _Intensity = 1.f;
	float _Range = 1.f;
	float _BoundingSphereOffsetAlongDir = 0.f;
	float _BoundingSphereRadius = 0.f;
	float _ConeInner = n_deg2rad(30.f); // In radians, full angle (not half), Theta
	float _ConeOuter = n_deg2rad(45.f); // In radians, full angle (not half), Phi
	float _SinHalfOuter = 0.f;
	float _CosHalfOuter = 0.f;
	float _CosHalfInner = 0.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual rtm::vector4f         GetLocalSphere() const override { return rtm::vector_set(0.f, 0.f, -_BoundingSphereOffsetAlongDir, _BoundingSphereRadius); }
	virtual bool                  GetLocalAABB(Math::CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(rtm::vector4f_arg0 Sphere) const override;
	virtual U8                    TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const override;
	virtual bool                  DoesEmitAnyEnergy() const override { return (_Color & 0x00ffffff) && _Intensity > 0.f; }
	virtual void                  RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
};

typedef Ptr<CSpotLightAttribute> PSpotLightAttribute;

}
