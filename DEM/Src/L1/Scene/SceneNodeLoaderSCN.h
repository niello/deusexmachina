#pragma once
#ifndef __DEM_L1_SCENE_NODE_LOADER_H__
#define __DEM_L1_SCENE_NODE_LOADER_H__

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

	virtual ~CSceneNodeLoaderSCN() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CSceneNodeLoaderSCN> PSceneNodeLoaderSCN;

}

#endif
