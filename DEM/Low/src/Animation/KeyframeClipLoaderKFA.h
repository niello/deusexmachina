#pragma once
#include <Resources/ResourceCreator.h>

// Loads keyframed animation clip from a native DEM 'kfa' format

namespace Resources
{

class CKeyframeClipLoaderKFA: public IResourceCreator
{
public:

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CKeyframeClipLoaderKFA> PKeyframeClipLoaderKFA;

}
