#pragma once
#include <Resources/ResourceLoader.h>

// Loads CEntityTemplate from HRD description with nesting support

namespace Resources
{

class CEntityTemplateLoader: public CResourceLoader
{
public:

	CEntityTemplateLoader(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CEntityTemplateLoader> PEntityTemplateLoader;

}
