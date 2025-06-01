#pragma once
#include <Resources/ResourceLoader.h>

// Loads navigation agent settings from DEM "hrd" format

namespace Resources
{

class CNavAgentSettingsLoaderHRD : public CResourceLoader
{
public:

	CNavAgentSettingsLoaderHRD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CNavAgentSettingsLoaderHRD> PNavAgentSettingsLoaderHRD;

}
