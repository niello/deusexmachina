#pragma once
#include <Frame/LightAttribute.h>
#include <Math/CameraMath.h> // Math::ClipInside

// Directional light in a scene

namespace Frame
{

class CDirectionalLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	U32   _Color = Render::Color_White;
	float _Intensity = 1.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual rtm::vector4f         GetLocalSphere() const override { return rtm::vector_set(0.f, 0.f, 0.f, -1.f); }
	virtual bool                  GetLocalAABB(Math::CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(rtm::vector4f_arg0 Sphere) const override { return true; }
	virtual U8                    TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const override { return Math::ClipInside; }
	virtual bool                  DoesEmitAnyEnergy() const override { return (_Color & 0x00ffffff) && _Intensity > 0.f; }
};

typedef Ptr<CDirectionalLightAttribute> PDirectionalLightAttribute;

}
