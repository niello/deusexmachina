#pragma once
#include <Frame/NodeAttrRenderable.h>

// Scene node attribute with renderable terrain

namespace Frame
{

class CTerrainAttribute: public CNodeAttrRenderable
{
	__DeclareClass(CTerrainAttribute);

protected:

	CStrID MaterialUID;
	CStrID CDLODDataUID;
	CStrID HeightMapUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateResources(CGraphicsResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;
};

typedef Ptr<CTerrainAttribute> PTerrainAttribute;

}