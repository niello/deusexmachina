#include "D3D9Shader.h"
#include <Render/ShaderParamTable.h>
#include "DEMD3D9.h"

namespace Render
{
__ImplementClassNoFactory(Render::CD3D9Shader, Render::CShader);

CD3D9Shader::CD3D9Shader(IDirect3DVertexShader9* pShader, PShaderParamTable Params)
	: _pShader(pShader)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Vertex : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D9Shader::CD3D9Shader(IDirect3DPixelShader9* pShader, PShaderParamTable Params)
	: _pShader(pShader)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Pixel : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D9Shader::~CD3D9Shader()
{
	SAFE_RELEASE(_pShader);
}
//---------------------------------------------------------------------

IDirect3DVertexShader9* CD3D9Shader::GetD3DVertexShader() const
{
	n_assert_dbg(_Type == ShaderType_Vertex);
	return (_Type == ShaderType_Vertex) ? static_cast<IDirect3DVertexShader9*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

IDirect3DPixelShader9* CD3D9Shader::GetD3DPixelShader() const
{
	n_assert_dbg(_Type == ShaderType_Pixel);
	return (_Type == ShaderType_Pixel) ? static_cast<IDirect3DPixelShader9*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

}