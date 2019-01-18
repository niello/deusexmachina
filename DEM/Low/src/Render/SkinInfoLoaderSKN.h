#pragma once
#ifndef __DEM_L1_SKIN_INFO_LOADER_SKN_H__
#define __DEM_L1_SKIN_INFO_LOADER_SKN_H__

#include <Resources/ResourceCreator.h>

// Loads CSkinInfo (skin & skeleton data) from a native DEM 'skn' format

namespace Resources
{

class CSkinInfoLoaderSKN: public IResourceCreator
{
	__DeclareClass(CSkinInfoLoaderSKN);

public:

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CSkinInfoLoaderSKN> PSkinInfoLoaderSKN;

}

#endif
