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
	virtual acl::Vector4_32       GetLocalSphere() const override { return acl::vector_set(0.f, 0.f, 0.f, -1.f); }
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(acl::Vector4_32Arg0 Sphere) const override { return true; }
	virtual U8                    TestBoxClipping(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const override { return Math::ClipInside; }
	virtual bool                  DoesEmitAnyEnergy() const override { return (_Color & 0x00ffffff) && _Intensity > 0.f; }
};

typedef Ptr<CDirectionalLightAttribute> PDirectionalLightAttribute;

}
