#include "D3D11ShaderLoaders.h"

#include <Resources/Resource.h>
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11Shader.h>
#include <IO/IOServer.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef GetObject

namespace Resources
{
__ImplementClass(Resources::CD3D11VertexShaderLoader, 'VSL1', Resources::CShaderLoader);
__ImplementClass(Resources::CD3D11PixelShaderLoader, 'PSL1', Resources::CShaderLoader);

///////////////////////////////////////////////////////////////////////
// CD3D11VertexShaderLoader
///////////////////////////////////////////////////////////////////////

const Core::CRTTI& CD3D11VertexShaderLoader::GetResultType() const
{
	return Render::CD3D11Shader::RTTI;
}
//---------------------------------------------------------------------

bool CD3D11VertexShaderLoader::IsProvidedDataValid() const
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!can read raw data to IO cache! or, if stream is mapped, can open and close stream outside these methods!
	//GPU->ValidateShaderBinary(ShaderType);
	//	return SUCCEEDED(pGPU->GetD3DDevice()->CreateVertexShader(buf, buflen, NULL, NULL));

	OK;
}
//---------------------------------------------------------------------

bool CD3D11VertexShaderLoader::Load(CResource& Resource)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!open stream from URI w/optional IO cache!
	Data::CBuffer Buf;
	if (!IOSrv->LoadFileToBuffer(Resource.GetUID().CStr(), Buf)) FAIL;

	Render::PShader Shader = GPU->CreateShader(Render::ShaderType_Vertex, Buf.GetPtr(), Buf.GetSize());
	if (Shader.IsNullPtr()) FAIL;

	Resource.Init(Shader.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////
// CD3D11PixelShaderLoader
///////////////////////////////////////////////////////////////////////

const Core::CRTTI& CD3D11PixelShaderLoader::GetResultType() const
{
	return Render::CD3D11Shader::RTTI;
}
//---------------------------------------------------------------------

bool CD3D11PixelShaderLoader::IsProvidedDataValid() const
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!can read raw data to IO cache! or, if stream is mapped, can open and close stream outside these methods!
	//GPU->ValidateShaderBinary(ShaderType);
	//	return SUCCEEDED(pGPU->GetD3DDevice()->CreatePixelShader(buf, buflen, NULL, NULL));

	OK;
}
//---------------------------------------------------------------------

bool CD3D11PixelShaderLoader::Load(CResource& Resource)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!open stream from URI w/optional IO cache!
	Data::CBuffer Buf;
	if (!IOSrv->LoadFileToBuffer(Resource.GetUID().CStr(), Buf)) FAIL;

	Render::PShader Shader = GPU->CreateShader(Render::ShaderType_Pixel, Buf.GetPtr(), Buf.GetSize());
	if (Shader.IsNullPtr()) FAIL;

	Resource.Init(Shader.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}