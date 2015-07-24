#include "D3D11ShaderLoaders.h"

#include <Resources/Resource.h>
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11Shader.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef GetObject

namespace Resources
{
__ImplementClassNoFactory(Resources::CD3D11VertexShaderLoader, Resources::CShaderLoader);

const Core::CRTTI& CD3D11VertexShaderLoader::GetResultType() const
{
	return Render::CD3D11Shader::RTTI;
}
//---------------------------------------------------------------------

bool CD3D11VertexShaderLoader::IsProvidedDataValid() const
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!can read raw data to IO cache! or, if stream is mapped, can open and close stream outside these methods!
	//return SUCCEEDED(pGPU->GetD3DDevice()->CreateVertexShader(buf, buflen, NULL, NULL));

	OK;
}
//---------------------------------------------------------------------

bool CD3D11VertexShaderLoader::Load(CResource& Resource)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	//!!!open stream from URI w/optional IO cache
	const void* pData = NULL;
	DWORD DataSize = 0;
	n_assert(pData && DataSize);

	Render::CD3D11GPUDriver* pGPU = (Render::CD3D11GPUDriver*)GPU.GetUnsafe();
	ID3D11VertexShader* pVS = NULL;
	bool Result = SUCCEEDED(pGPU->GetD3DDevice()->CreateVertexShader(pData, DataSize, NULL, &pVS));

	Render::PD3D11Shader Shader = n_new(Render::CD3D11Shader);
	if (!Shader->Create(pVS))
	{
		pVS->Release();
		FAIL;
	}

	Resource.Init(Shader.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}