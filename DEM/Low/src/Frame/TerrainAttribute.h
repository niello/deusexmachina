#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable terrain

namespace Frame
{

class CTerrainAttribute: public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID MaterialUID;
	CStrID CDLODDataUID;
	CStrID HeightMapUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual bool                  ValidateGPUResources(CGraphicsResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
};

typedef Ptr<CTerrainAttribute> PTerrainAttribute;

}
