#pragma once
#ifndef __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__
#define __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__

#include <Resources/ResourceCreator.h>

// Loads CShaderLibrary (packed and indexed shaders) from a native DEM 'slb' format

namespace Resources
{

class CShaderLibraryLoaderSLB: public IResourceCreator
{
	__DeclareClass(CShaderLibraryLoaderSLB);

public:

	//virtual ~CShaderLibraryLoaderSLB() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CShaderLibraryLoaderSLB> PShaderLibraryLoaderSLB;

}

#endif
