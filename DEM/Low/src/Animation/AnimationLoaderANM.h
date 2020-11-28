#pragma once
#include <Resources/ResourceLoader.h>

// Loads CAnimationClip from native DEM "anm" format (ACL-compressed)

namespace Resources
{

class CAnimationLoaderANM: public CResourceLoader
{
public:

	CAnimationLoaderANM(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual Core::PObject      CreateResource(CStrID UID) override;
};

typedef Ptr<CAnimationLoaderANM> PAnimationLoaderANM;

}
