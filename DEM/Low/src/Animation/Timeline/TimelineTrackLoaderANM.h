#pragma once
#include <Resources/ResourceLoader.h>

// Loads timeline track from a single animation clip in DEM .anm format.
// Asset will consist of a single pose track with animated pose clip,
// all settings are default. Use this loader to play simple animations.

namespace Resources
{

class CTimelineTrackLoaderANM : public CResourceLoader
{
public:

	CTimelineTrackLoaderANM(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

typedef Ptr<CTimelineTrackLoaderANM> PTimelineTrackLoaderANM;

}
