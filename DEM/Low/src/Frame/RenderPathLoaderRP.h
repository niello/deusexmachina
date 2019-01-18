#pragma once
#ifndef __DEM_L1_RENDER_PATH_LOADER_RP_H__
#define __DEM_L1_RENDER_PATH_LOADER_RP_H__

#include <Resources/ResourceCreator.h>

// Loads a render path from DEM (.rp) format

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Resources
{

class CRenderPathLoaderRP: public IResourceCreator
{
	__DeclareClass(CRenderPathLoaderRP);

public:

	//virtual ~CRenderPathLoaderRP() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CRenderPathLoaderRP> PRenderPathLoaderRP;

}

#endif
