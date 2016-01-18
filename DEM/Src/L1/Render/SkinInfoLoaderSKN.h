#pragma once
#ifndef __DEM_L1_SKIN_INFO_LOADER_SKN_H__
#define __DEM_L1_SKIN_INFO_LOADER_SKN_H__

#include <Resources/ResourceLoader.h>

// Loads CSkinInfo (skin & skeleton data) from a native DEM 'skn' format

namespace Resources
{

class CSkinInfoLoaderSKN: public CResourceLoader
{
	__DeclareClass(CSkinInfoLoaderSKN);

public:

	//virtual ~CSkinInfoLoaderSKN() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CSkinInfoLoaderSKN> PSkinInfoLoaderSKN;

}

#endif
