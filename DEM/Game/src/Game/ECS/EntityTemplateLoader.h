#pragma once
#include <Resources/ResourceLoader.h>

// Loads CEntityTemplate from HRD description with nesting support

namespace Resources
{

class CEntityTemplateLoader: public CResourceLoader
{
public:

	CEntityTemplateLoader(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CEntityTemplateLoader> PEntityTemplateLoader;

}
