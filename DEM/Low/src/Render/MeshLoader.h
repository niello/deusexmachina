#pragma once
#include <Resources/ResourceLoader.h>

// A class of loaders that load CMesh objects from different data formats

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CMeshLoader: public CResourceLoader
{
public:

	Render::PGPUDriver GPU;

	virtual ~CMeshLoader();

	virtual const Core::CRTTI&	GetResultType() const override;
};

typedef Ptr<CMeshLoader> PMeshLoader;

}
