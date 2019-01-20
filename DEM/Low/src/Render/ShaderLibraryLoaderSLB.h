#pragma once
#include <Resources/ResourceLoader.h>

// Loads CShaderLibrary (packed and indexed shaders) from a native DEM 'slb' format

namespace Resources
{

class CShaderLibraryLoaderSLB: public CResourceLoader
{
public:

	CShaderLibraryLoaderSLB(IO::CIOServer* pIOServer) : CResourceLoader(pIOServer) {}

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CShaderLibraryLoaderSLB> PShaderLibraryLoaderSLB;

}
