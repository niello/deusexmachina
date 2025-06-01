#pragma once
#include <Resources/ResourceLoader.h>

// Loads timeline track from HRD description

namespace Resources
{

class CTimelineTrackLoaderHRD : public CResourceLoader
{
public:

	CTimelineTrackLoaderHRD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

typedef Ptr<CTimelineTrackLoaderHRD> PTimelineTrackLoaderHRD;

}
