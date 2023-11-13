#pragma once
#include <Frame/LightAttribute.h>
#include <Render/Texture.h>

// Image-based light (IBL) source in a scene

namespace Render
{
	typedef Ptr<class CTexture> PTexture;
}

namespace Frame
{

class CIBLAmbientLightAttribute: public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID _IrradianceMapUID;
	CStrID _RadianceEnvMapUID;
	float  _Range = -1.f;      // Negative value means a global omnipresent light source

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual rtm::vector4f         GetLocalSphere() const override { return rtm::vector_set(0.f, 0.f, 0.f, _Range); }
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
	virtual bool                  IntersectsWith(rtm::vector4f_arg0 Sphere) const override;
	virtual U8                    TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const override;
	virtual bool                  DoesEmitAnyEnergy() const override { return _IrradianceMapUID || _RadianceEnvMapUID; }

	bool                          IsGlobal() const { return std::signbit(_Range); }
};

typedef Ptr<CIBLAmbientLightAttribute> PIBLAmbientLightAttribute;

}
