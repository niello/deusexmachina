#pragma once
#ifndef __DEM_L1_CDLOD_DATA_LOADER_H__
#define __DEM_L1_CDLOD_DATA_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads CDLOD terrain rendering data from DEM "cdlod" format

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CCDLODDataLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU; // For heightfield texture only

	virtual ~CCDLODDataLoader() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CCDLODDataLoader> PCDLODDataLoader;

}

#endif
