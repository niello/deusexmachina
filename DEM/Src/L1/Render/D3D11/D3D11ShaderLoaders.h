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
	__DeclareClass(CD3D11ShaderLoader);

protected:

	PResourceObject						LoadImpl(IO::CStream& Stream, Render::EShaderType ShaderType);

public:

	virtual bool						IsProvidedDataValid() const { OK; } //!!!???write?!
	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_RANDOM; }
	virtual PResourceObject				Load(IO::CStream& Stream) { return LoadImpl(Stream, Render::ShaderType_Unknown); }
};

}

#endif
