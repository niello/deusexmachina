#pragma once
#include <Resources/ResourceLoader.h>

// Loads scene node hierarchy and attributes from DEM "scn" format

namespace Resources
{

class CSceneNodeLoaderSCN: public CResourceLoader
{
public:

	CSceneNodeLoaderSCN(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual Core::PObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CSceneNodeLoaderSCN> PSceneNodeLoaderSCN;

}
