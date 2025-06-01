#pragma once
#include <Resources/ResourceLoader.h>

// Loads scene node hierarchy and attributes from DEM "scn" format

namespace Resources
{

class CSceneNodeLoaderSCN: public CResourceLoader
{
public:

	CSceneNodeLoaderSCN(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI&			GetResultType() const override;
	virtual DEM::Core::PObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CSceneNodeLoaderSCN> PSceneNodeLoaderSCN;

}
