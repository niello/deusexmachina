#pragma once
#ifndef __DEM_L1_D3D11_SHADER_LOADERS_H__
#define __DEM_L1_D3D11_SHADER_LOADERS_H__

#include <Render/ShaderLoader.h>
#include <Render/RenderFwd.h>

// Direct3D11 shader loader implementations for different shader types.
// Uses engine-specific data format.

namespace Resources
{

class CD3D11ShaderLoader: public CShaderLoader
{
protected:

	PResourceObject						LoadImpl(CStrID UID, Render::EShaderType ShaderType);

public:

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) { return LoadImpl(UID, Render::ShaderType_Unknown); }
};

}

#endif
