#pragma once
#ifndef __DEM_L1_MESH_LOADER_H__
#define __DEM_L1_MESH_LOADER_H__

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
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU;

	virtual ~CMeshLoader();

	virtual const Core::CRTTI&	GetResultType() const;
};

typedef Ptr<CMeshLoader> PMeshLoader;

}

#endif
