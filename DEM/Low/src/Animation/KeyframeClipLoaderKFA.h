#pragma once
#include <Resources/ResourceCreator.h>

// Loads keyframed animation clip from a native DEM 'kfa' format

namespace Resources
{

class CKeyframeClipLoaderKFA: public IResourceCreator
{
public:

	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CKeyframeClipLoaderKFA> PKeyframeClipLoaderKFA;

}
