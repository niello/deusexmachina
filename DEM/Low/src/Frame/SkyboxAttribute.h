#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable skybox

namespace Frame
{

class CSkyboxAttribute: public CRenderableAttribute
{
	__DeclareClass(CSkyboxAttribute);

protected:

	CStrID MaterialUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateGPUResources(CGraphicsResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
};

typedef Ptr<CSkyboxAttribute> PSkyboxAttribute;

}
