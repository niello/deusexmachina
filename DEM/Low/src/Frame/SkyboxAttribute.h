#pragma once
#include <Frame/NodeAttrRenderable.h>

// Scene node attribute with renderable skybox

namespace Frame
{

class CSkyboxAttribute: public CNodeAttrRenderable
{
	__DeclareClass(CSkyboxAttribute);

protected:

	CStrID MaterialUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateResources(CGraphicsResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
};

typedef Ptr<CSkyboxAttribute> PSkyboxAttribute;

}
