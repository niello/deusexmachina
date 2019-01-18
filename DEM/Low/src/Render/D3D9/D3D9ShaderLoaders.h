#pragma once
#ifndef __DEM_L1_D3D9_SHADER_LOADERS_H__
#define __DEM_L1_D3D9_SHADER_LOADERS_H__

#include <Render/ShaderLoader.h>
#include <Render/RenderFwd.h>

// Direct3D9 shader loader implementations for different shader types.
// Uses engine-specific data format.

namespace Resources
{

class CD3D9ShaderLoader: public CShaderLoader
{
	__DeclareClass(CD3D9ShaderLoader);

protected:

	PResourceObject				LoadImpl(CStrID UID, Render::EShaderType ShaderType);

public:

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID) { return LoadImpl(UID, Render::ShaderType_Unknown); }
};

typedef Ptr<CD3D9ShaderLoader> PD3D9ShaderLoader;

}

#endif
