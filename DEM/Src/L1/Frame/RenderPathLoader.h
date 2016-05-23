#pragma once
#ifndef __DEM_L1_RENDER_PATH_LOADER_H__
#define __DEM_L1_RENDER_PATH_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads a render path from HRD or PRM

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Resources
{

class CRenderPathLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

protected:

	virtual PResourceObject				LoadImpl(Data::PParams Desc);

public:

	//virtual ~CRenderPathLoader() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
};

typedef Ptr<CRenderPathLoader> PRenderPathLoader;

class CRenderPathLoaderHRD: public CRenderPathLoader
{
	__DeclareClass(CRenderPathLoaderHRD);

public:

	//virtual ~CRenderPathLoaderHRD() {}

	virtual PResourceObject	Load(IO::CStream& Stream);
};

typedef Ptr<CRenderPathLoaderHRD> PRenderPathLoaderHRD;

class CRenderPathLoaderPRM: public CRenderPathLoader
{
	__DeclareClass(CRenderPathLoaderPRM);

public:

	//virtual ~CRenderPathLoaderPRM() {}

	virtual PResourceObject	Load(IO::CStream& Stream);
};

typedef Ptr<CRenderPathLoaderPRM> PRenderPathLoaderPRM;

}

#endif
