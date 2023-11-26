#pragma once
#include <Frame/LightAttribute.h>

// Point light in a scene

namespace Frame
{

class CPointLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

public:

	U32   _Color = Render::Color_White;
	float _Intensity = 1.f;
	float _Range = 1.f;

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual rtm::vector4f         GetLocalSphere() const override { return rtm::vector_set(0.f, 0.f, 0.f, _Range); }
	virtual bool                  GetLocalAABB(Math::CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(rtm::vector4f_arg0 Sphere) const override;
	virtual U8                    TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const override;
	virtual bool                  DoesEmitAnyEnergy() const override { return (_Color & 0x00ffffff) && _Intensity > 0.f; }
	virtual void                  RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
};

typedef Ptr<CPointLightAttribute> PPointLightAttribute;

}
