#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable model

namespace Frame
{

class CModelAttribute : public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID _MeshUID;
	CStrID _MaterialUID;

public:

	CModelAttribute() = default;
	CModelAttribute(CStrID MeshUID, CStrID MaterialUID, U32 MeshGroupIndex = 0);

	virtual bool LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual bool ValidateGPUResources(CGraphicsResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
};

typedef Ptr<CModelAttribute> PModelAttribute;

}
