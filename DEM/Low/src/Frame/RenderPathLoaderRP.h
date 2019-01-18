#pragma once
#ifndef __DEM_L1_RENDER_PATH_LOADER_RP_H__
#define __DEM_L1_RENDER_PATH_LOADER_RP_H__

#include <Resources/ResourceLoader.h>

// Loads a render path from DEM (.rp) format

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Resources
{

class CRenderPathLoaderRP: public CResourceLoader
{
public:

	//virtual ~CRenderPathLoaderRP() {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CRenderPathLoaderRP> PRenderPathLoaderRP;

}

#endif
