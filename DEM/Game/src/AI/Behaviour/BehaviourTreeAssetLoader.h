#pragma once
#include <Resources/ResourceLoader.h>

// Loads a BehaviourTree asset from a file

namespace Resources
{
using PBehaviourTreeAssetLoader = Ptr<class CBehaviourTreeAssetLoader>;

class CBehaviourTreeAssetLoader : public CResourceLoader
{
public:

	using CResourceLoader::CResourceLoader;

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

}
