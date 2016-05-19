#pragma once
#ifndef __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__
#define __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__

#include <Resources/ResourceLoader.h>

// Loads CShaderLibrary (packed and indexed shaders) from a native DEM 'slb' format

namespace Resources
{

class CShaderLibraryLoaderSLB: public CResourceLoader
{
	__DeclareClass(CShaderLibraryLoaderSLB);

public:

	//virtual ~CShaderLibraryLoaderSLB() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CShaderLibraryLoaderSLB> PShaderLibraryLoaderSLB;

}

#endif
