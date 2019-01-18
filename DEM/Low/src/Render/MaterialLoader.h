#pragma once
#include <Resources/ResourceLoader.h>
#include <Data/Dictionary.h>

// Loads render material from DEM "prm" format

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CMaterialLoader: public CResourceLoader
{
public:

	Render::PGPUDriver GPU;

	virtual ~CMaterialLoader() {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CMaterialLoader> PMaterialLoader;

}
