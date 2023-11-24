#include "D3D11Shader.h"
#include <Render/ShaderParamTable.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{

CD3D11Shader::CD3D11Shader(ID3D11VertexShader* pShader, U32 InputSignatureID, PShaderParamTable Params)
	: _pShader(pShader)
	, _InputSignatureID(InputSignatureID)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Vertex : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D11Shader::CD3D11Shader(ID3D11PixelShader* pShader, PShaderParamTable Params)
	: _pShader(pShader)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Pixel : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D11Shader::CD3D11Shader(ID3D11GeometryShader* pShader, U32 InputSignatureID, PShaderParamTable Params)
	: _pShader(pShader)
	, _InputSignatureID(InputSignatureID)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Geometry : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D11Shader::CD3D11Shader(ID3D11HullShader* pShader, PShaderParamTable Params)
	: _pShader(pShader)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Hull : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D11Shader::CD3D11Shader(ID3D11DomainShader* pShader, PShaderParamTable Params)
	: _pShader(pShader)
{
	n_assert(pShader);
	_Type = pShader ? ShaderType_Domain : ShaderType_Invalid;
	_Params = Params;
}
//---------------------------------------------------------------------

CD3D11Shader::~CD3D11Shader()
{
	SAFE_RELEASE(_pShader);
}
//---------------------------------------------------------------------

ID3D11VertexShader* CD3D11Shader::GetD3DVertexShader() const
{
	n_assert_dbg(_Type == ShaderType_Vertex);
	return (_Type == ShaderType_Vertex) ? static_cast<ID3D11VertexShader*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

ID3D11HullShader* CD3D11Shader::GetD3DHullShader() const
{
	n_assert_dbg(_Type == ShaderType_Hull);
	return (_Type == ShaderType_Hull) ? static_cast<ID3D11HullShader*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

ID3D11DomainShader* CD3D11Shader::GetD3DDomainShader() const
{
	n_assert_dbg(_Type == ShaderType_Domain);
	return (_Type == ShaderType_Domain) ? static_cast<ID3D11DomainShader*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

ID3D11GeometryShader* CD3D11Shader::GetD3DGeometryShader() const
{
	n_assert_dbg(_Type == ShaderType_Geometry);
	return (_Type == ShaderType_Geometry) ? static_cast<ID3D11GeometryShader*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

ID3D11PixelShader* CD3D11Shader::GetD3DPixelShader() const
{
	n_assert_dbg(_Type == ShaderType_Pixel);
	return (_Type == ShaderType_Pixel) ? static_cast<ID3D11PixelShader*>(_pShader) : nullptr;
}
//---------------------------------------------------------------------

void CD3D11Shader::SetDebugName(std::string_view Name)
{
#if DEM_RENDER_DEBUG
	if (_pShader) _pShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)Name.size(), Name.data());
#endif
}
//---------------------------------------------------------------------

}
