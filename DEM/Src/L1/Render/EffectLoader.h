#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads shader effect file in DEM (.eff) format

//namespace Render
//{
//	typedef Ptr<class CGPUDriver> PGPUDriver;
//}

namespace Resources
{

class CEffectLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	//Render::PGPUDriver GPU;

	//virtual ~CShaderLoader() {}

	//virtual const Core::CRTTI&	GetResultType() const;
};

typedef Ptr<CEffectLoader> PEffectLoader;

}

#endif
