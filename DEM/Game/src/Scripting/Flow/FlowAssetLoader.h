#pragma once
#include <Resources/ResourceLoader.h>

// Loads a Flow asset from a file

namespace Resources
{
using PFlowAssetLoader = Ptr<class CFlowAssetLoader>;

class CFlowAssetLoader : public CResourceLoader
{
public:

	using CResourceLoader::CResourceLoader;

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

}
