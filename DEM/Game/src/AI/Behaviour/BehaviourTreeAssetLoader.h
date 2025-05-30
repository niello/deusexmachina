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

	virtual const Core::CRTTI& GetResultType() const override;
	virtual Core::PObject      CreateResource(CStrID UID) override;
};

}
