#pragma once
#include <Frame/LightAttribute.h>

// Directional light in a scene

namespace Frame
{

class CDirectionalLightAttribute : public CLightAttribute
{
	FACTORY_CLASS_DECL;

protected:

	U32   _Color = 0xffffffff;
	float _Intensity = 1.f;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  GetLocalAABB(CAABB& OutBox) const override;
};

typedef Ptr<CDirectionalLightAttribute> PDirectionalLightAttribute;

}
