#pragma once
#ifndef __DEM_L1_SHADER_LOADER_H__
#define __DEM_L1_SHADER_LOADER_H__

#include <Resources/ResourceLoader.h>

// Shader loaders are intended to load GPU shader objects. It is a class of
// different loaders each suited for a different shader type and 3D API.

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CShaderLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU;

	//virtual ~CShaderLoader() {}

	//virtual const Core::CRTTI&	GetResultType() const;
};

typedef Ptr<CShaderLoader> PShaderLoader;

}

#endif
