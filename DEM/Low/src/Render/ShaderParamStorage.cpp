#include "ShaderParamStorage.h"
#include <Render/ShaderParamTable.h>
#include <Render/GPUDriver.h>
#include <Render/ConstantBuffer.h>
#include <Render/Texture.h>
#include <Render/Sampler.h>

namespace Render
{

CShaderParamStorage::CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU)
	: _Table(&Table)
	, _GPU(&GPU)
{
	_ConstantBuffers.resize(Table.GetConstantBuffers().size());
	_Resources.resize(Table.GetResources().size());
	_Samplers.resize(Table.GetSamplers().size());
}
//---------------------------------------------------------------------

CShaderParamStorage::~CShaderParamStorage() {}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetResource(CStrID ID, CTexture* pTexture)
{
	return SetResource(_Table->GetResourceIndex(ID), pTexture);
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetResource(size_t Index, CTexture* pTexture)
{
	if (Index >= _Resources.size()) FAIL;
	_Resources[Index] = pTexture;
	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetSampler(CStrID ID, CSampler* pSampler)
{
	return SetSampler(_Table->GetSamplerIndex(ID), pSampler);
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetSampler(size_t Index, CSampler* pSampler)
{
	if (Index >= _Samplers.size()) FAIL;
	_Samplers[Index] = pSampler;
	OK;
}
//---------------------------------------------------------------------

//???apply nullptrs too?
bool CShaderParamStorage::Apply() const
{
	for (size_t i = 0; i < _ConstantBuffers.size(); ++i)
	{
		if (_ConstantBuffers[i])
			if (!_Table->GetConstantBuffer(i)->Apply(*_GPU, _ConstantBuffers[i])) return false;
	}

	for (size_t i = 0; i < _Resources.size(); ++i)
	{
		if (_Resources[i])
			if (!_Table->GetResource(i)->Apply(*_GPU, _Resources[i])) return false;
	}

	for (size_t i = 0; i < _Samplers.size(); ++i)
	{
		if (_Samplers[i])
			if (!_Table->GetSampler(i)->Apply(*_GPU, _Samplers[i])) return false;
	}

	return true;
}
//---------------------------------------------------------------------

}