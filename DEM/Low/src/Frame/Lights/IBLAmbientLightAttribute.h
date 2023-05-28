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
	float  _Range = -1.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;

	bool                          IsGlobal() const { return std::signbit(_Range); }
};

typedef Ptr<CIBLAmbientLightAttribute> PIBLAmbientLightAttribute;

}
