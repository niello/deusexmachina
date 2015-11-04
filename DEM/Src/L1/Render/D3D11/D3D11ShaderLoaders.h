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

	bool						LoadImpl(CResource& Resource, Render::EShaderType ShaderType);

public:

	virtual const Core::CRTTI&	GetResultType() const;
};

class CD3D11VertexShaderLoader: public CD3D11ShaderLoader
{
	__DeclareClass(CD3D11VertexShaderLoader);

public:

	virtual bool	IsProvidedDataValid() const;
	virtual bool	Load(CResource& Resource) { return LoadImpl(Resource, Render::ShaderType_Vertex); }
};

typedef Ptr<CD3D11VertexShaderLoader> PD3D11VertexShaderLoader;

class CD3D11PixelShaderLoader: public CD3D11ShaderLoader
{
	__DeclareClass(CD3D11PixelShaderLoader);

public:

	virtual bool	IsProvidedDataValid() const;
	virtual bool	Load(CResource& Resource) { return LoadImpl(Resource, Render::ShaderType_Pixel); }
};

typedef Ptr<CD3D11PixelShaderLoader> PD3D11PixelShaderLoader;

}

#endif
