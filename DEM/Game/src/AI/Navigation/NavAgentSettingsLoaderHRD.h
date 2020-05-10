#pragma once
#include <Resources/ResourceLoader.h>

// Loads navigation agent settings from DEM "hrd" format

namespace Resources
{

class CNavAgentSettingsLoaderHRD : public CResourceLoader
{
public:

	CNavAgentSettingsLoaderHRD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual PResourceObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CNavAgentSettingsLoaderHRD> PNavAgentSettingsLoaderHRD;

}
