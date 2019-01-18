#pragma once
#ifndef __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__
#define __DEM_L1_SHADER_LIBRARY_LOADER_SLB_H__

#include <Resources/ResourceCreator.h>

// Loads CShaderLibrary (packed and indexed shaders) from a native DEM 'slb' format

namespace Resources
{

class CShaderLibraryLoaderSLB: public IResourceCreator
{
public:

	//virtual ~CShaderLibraryLoaderSLB() {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CShaderLibraryLoaderSLB> PShaderLibraryLoaderSLB;

}

#endif
