#pragma once
#include <Frame/LightAttribute.h>

// Point light in a scene

namespace Frame
{

class CPointLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	U32   _Color = 0xffffffff;
	float _Intensity = 1.f;
	float _Range = 1.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PLight        CreateLight() const override;
	virtual void                  UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
};

typedef Ptr<CPointLightAttribute> PPointLightAttribute;

}
