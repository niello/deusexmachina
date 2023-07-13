#include "ShaderParamStorage.h"
#include <Render/ShaderParamTable.h>
#include <Render/GPUDriver.h>
#include <Render/ConstantBuffer.h>
#include <Render/Texture.h>
#include <Render/Sampler.h>

namespace Render
{

CShaderParamStorage::CShaderParamStorage() = default;
//---------------------------------------------------------------------

CShaderParamStorage::CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU, bool UnapplyOnDestruction)
	: _Table(&Table)
	, _GPU(&GPU)
	, _UnapplyOnDestruction(UnapplyOnDestruction)
{
	_ConstantBuffers.resize(Table.GetConstantBuffers().size());
	_Resources.resize(Table.GetResources().size());
	_Samplers.resize(Table.GetSamplers().size());
}
//---------------------------------------------------------------------

CShaderParamStorage::CShaderParamStorage(CShaderParamStorage&& Other) = default;
//---------------------------------------------------------------------

CShaderParamStorage& CShaderParamStorage::operator =(CShaderParamStorage&& Other) = default;
//---------------------------------------------------------------------

CShaderParamStorage::~CShaderParamStorage()
{
	if (_UnapplyOnDestruction) Unapply();
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetConstantBuffer(CStrID ID, CConstantBuffer* pBuffer)
{
	return SetConstantBuffer(_Table->GetConstantBufferIndex(ID), pBuffer);
}
//---------------------------------------------------------------------

//???try to keep values from the previous buffer? or do externally if required?
bool CShaderParamStorage::SetConstantBuffer(size_t Index, CConstantBuffer* pBuffer)
{
	if (Index >= _ConstantBuffers.size()) FAIL;

	auto pCurrCB = _ConstantBuffers[Index].Get();
	if (pCurrCB && pCurrCB->IsTemporary())
		_GPU->FreeTemporaryConstantBuffer(*pCurrCB);

	_ConstantBuffers[Index] = pBuffer;

	OK;
}
//---------------------------------------------------------------------

CConstantBuffer* CShaderParamStorage::CreatePermanentConstantBuffer(size_t Index, U8 AccessFlags)
{
	if (Index >= _ConstantBuffers.size()) return nullptr;

	//???if exists, assert access flags or even replace with new buffer?
	if (!_ConstantBuffers[Index])
		_ConstantBuffers[Index] = _GPU->CreateConstantBuffer(*_Table->GetConstantBuffer(Index), AccessFlags);

	return _ConstantBuffers[Index];
}
//---------------------------------------------------------------------

CConstantBuffer* CShaderParamStorage::GetConstantBuffer(size_t Index, bool Create)
{
	if (Index >= _ConstantBuffers.size()) return nullptr;

	// Check if explicit buffer is not set and temporary buffer is not created yet
	if (!_ConstantBuffers[Index] && Create)
		_ConstantBuffers[Index] = _GPU->CreateTemporaryConstantBuffer(*_Table->GetConstantBuffer(Index));

	return _ConstantBuffers[Index];
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetRawConstant(const CShaderConstantParam& Param, const void* pData, UPTR Size)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetRawValue(*pCB, pData, Size);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetFloat(const CShaderConstantParam& Param, float Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetFloat(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetInt(const CShaderConstantParam& Param, I32 Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetInt(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetUInt(const CShaderConstantParam& Param, U32 Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetUInt(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetVector(const CShaderConstantParam& Param, const vector2& Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetVector(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetVector(const CShaderConstantParam& Param, const vector3& Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetVector(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetVector(const CShaderConstantParam& Param, const vector4& Value)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetVector(*pCB, Value);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetMatrix(const CShaderConstantParam& Param, const matrix44& Value, bool ColumnMajor)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetMatrix(*pCB, Value, ColumnMajor);

	OK;
}
//---------------------------------------------------------------------

bool CShaderParamStorage::SetMatrixArray(const CShaderConstantParam& Param, const matrix44* pValues, UPTR Count, U32 StartIndex, bool ColumnMajor)
{
	if (!Param) FAIL;
	auto* pCB = GetConstantBuffer(Param.GetConstantBufferIndex());
	if (!pCB) FAIL;

	if (!pCB->IsInWriteMode())
		_GPU->BeginShaderConstants(*pCB);

	Param.SetMatrixArray(*pCB, pValues, Count, StartIndex, ColumnMajor);

	OK;
}
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
bool CShaderParamStorage::Apply()
{
	for (size_t i = 0; i < _ConstantBuffers.size(); ++i)
	{
		if (auto* pCB = _ConstantBuffers[i].Get())
		{
			//???only if pCB->IsDirty()?
			if (pCB->IsInWriteMode())
				_GPU->CommitShaderConstants(*pCB);

			if (!_Table->GetConstantBuffer(i)->Apply(*_GPU, _ConstantBuffers[i])) return false;

			if (pCB->IsTemporary())
			{
				//!!!must return to the pool only when GPU is finished with this buffer!
				_GPU->FreeTemporaryConstantBuffer(*pCB);
				_ConstantBuffers[i] = nullptr;
			}
		}
	}

	for (size_t i = 0; i < _Resources.size(); ++i)
		if (_Resources[i])
			if (!_Table->GetResource(i)->Apply(*_GPU, _Resources[i])) return false;

	for (size_t i = 0; i < _Samplers.size(); ++i)
		if (_Samplers[i])
			if (!_Table->GetSampler(i)->Apply(*_GPU, _Samplers[i])) return false;

	return true;
}
//---------------------------------------------------------------------

void CShaderParamStorage::Unapply()
{
	// Don't unbind temporary buffers. If it was applied it would be cleared from here.
	for (size_t i = 0; i < _ConstantBuffers.size(); ++i)
		if (_ConstantBuffers[i] && !_ConstantBuffers[i]->IsTemporary())
			_Table->GetConstantBuffer(i)->Unapply(*_GPU, _ConstantBuffers[i]);

	for (size_t i = 0; i < _Resources.size(); ++i)
		if (_Resources[i])
			_Table->GetResource(i)->Unapply(*_GPU, _Resources[i]);

	for (size_t i = 0; i < _Samplers.size(); ++i)
		if (_Samplers[i])
			_Table->GetSampler(i)->Unapply(*_GPU, _Samplers[i]);
}
//---------------------------------------------------------------------

}
