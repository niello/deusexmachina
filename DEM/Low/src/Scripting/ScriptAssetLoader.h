#pragma once
#include <Resources/ResourceLoader.h>

// Loads a Lua script from the source or binary file

namespace Resources
{

class CScriptAssetLoader: public CResourceLoader
{
public:

	using CResourceLoader::CResourceLoader;

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

typedef Ptr<CScriptAssetLoader> PScriptAssetLoaderLoader;

}
