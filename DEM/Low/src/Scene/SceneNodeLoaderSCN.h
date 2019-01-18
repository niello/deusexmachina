#pragma once
#include <Resources/ResourceLoader.h>

// Loads scene node hierarchy and attributes from DEM "scn" format

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Resources
{

class CSceneNodeLoaderSCN: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	//???!!!TMP!? Doesn't fit very good into a resource system
	Scene::PSceneNode RootNode;	// If not NULL, root node data will be loaded into it instead of allocating a new node

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CSceneNodeLoaderSCN> PSceneNodeLoaderSCN;

}
