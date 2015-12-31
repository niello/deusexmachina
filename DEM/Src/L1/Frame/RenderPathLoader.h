#pragma once
#ifndef __DEM_L1_RENDER_PATH_LOADER_H__
#define __DEM_L1_RENDER_PATH_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads a render path from HRD or PRM

namespace Resources
{

class CRenderPathLoader: public CResourceLoader
{
	__DeclareClass(CRenderPathLoader);

public:

	virtual ~CRenderPathLoader() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CRenderPathLoader> PRenderPathLoader;

}

#endif
