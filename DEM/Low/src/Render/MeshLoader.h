#pragma once
#ifndef __DEM_L1_MESH_LOADER_H__
#define __DEM_L1_MESH_LOADER_H__

#include <Resources/ResourceCreator.h>

// A class of loaders that load CMesh objects from different data formats

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CMeshLoader: public IResourceCreator
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU;

	virtual ~CMeshLoader();

	virtual const Core::CRTTI&	GetResultType() const override;
};

typedef Ptr<CMeshLoader> PMeshLoader;

}

#endif
