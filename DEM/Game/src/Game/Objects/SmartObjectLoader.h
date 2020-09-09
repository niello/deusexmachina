#pragma once
#include <Resources/ResourceLoader.h>

// Loads CSmartObject from HRD description

namespace Resources
{

class CSmartObjectLoader: public CResourceLoader
{
public:

	CSmartObjectLoader(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual PResourceObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CSmartObjectLoader> PSmartObjectLoader;

}
