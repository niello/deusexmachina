#pragma once
#ifndef __DEM_L1_CDLOD_DATA_LOADER_H__
#define __DEM_L1_CDLOD_DATA_LOADER_H__

#include <Resources/ResourceCreator.h>

// Loads CDLOD terrain rendering data from DEM "cdlod" format

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CCDLODDataLoader: public IResourceCreator
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU; // For heightfield texture only

	virtual ~CCDLODDataLoader() {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CCDLODDataLoader> PCDLODDataLoader;

}

#endif
