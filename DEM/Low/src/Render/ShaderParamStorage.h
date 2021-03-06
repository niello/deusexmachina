#pragma once
#include <Render/ShaderParamTable.h>

// Shader parameter storage stores values in a form ready to be committed to the GPU.
// In a combination with shader parameter table this class simplifies the process of
// setting shader parameters from the code.
// Indices are stable and can be used as handles for fast parameter setting.
// Constant buffers are managed automatically. User can set the buffer explicitly
// or a temporary one will be created at the first time it is required.

namespace Render
{

class CShaderParamStorage final
{
protected:

	PShaderParamTable            _Table;
	PGPUDriver                   _GPU;
	bool                         _UnapplyOnDestruction;

	std::vector<PConstantBuffer> _ConstantBuffers;
	std::vector<PTexture>        _Resources;
	std::vector<PSampler>        _Samplers;

public:

	CShaderParamStorage();
	CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU, bool UnapplyOnDestruction = false);
	CShaderParamStorage(CShaderParamStorage&& Other);
	~CShaderParamStorage();

	CShaderParamStorage& operator =(CShaderParamStorage&& Other);

	bool                     SetConstantBuffer(CStrID ID, CConstantBuffer* pBuffer);
	bool                     SetConstantBuffer(size_t Index, CConstantBuffer* pBuffer);
	CConstantBuffer*         CreatePermanentConstantBuffer(size_t Index, U8 AccessFlags);
	CConstantBuffer*         GetConstantBuffer(size_t Index, bool Create = true);

	bool                     SetRawConstant(CStrID ID, const void* pData, UPTR Size) { return SetRawConstant(_Table->GetConstant(ID), pData, Size); }
	bool                     SetRawConstant(size_t Index, const void* pData, UPTR Size) { return SetRawConstant(_Table->GetConstant(Index), pData, Size); }
	bool                     SetRawConstant(const CShaderConstantParam& Param, const void* pData, UPTR Size);
	bool                     SetFloat(const CShaderConstantParam& Param, float Value);
	bool                     SetInt(const CShaderConstantParam& Param, I32 Value);
	bool                     SetUInt(const CShaderConstantParam& Param, U32 Value);
	bool                     SetVector(const CShaderConstantParam& Param, const vector2& Value);
	bool                     SetVector(const CShaderConstantParam& Param, const vector3& Value);
	bool                     SetVector(const CShaderConstantParam& Param, const vector4& Value);
	bool                     SetMatrix(const CShaderConstantParam& Param, const matrix44& Value, bool ColumnMajor = false);
	bool                     SetMatrixArray(const CShaderConstantParam& Param, const matrix44* pValues, UPTR Count, bool ColumnMajor = false);

	bool                     SetResource(CStrID ID, CTexture* pTexture);
	bool                     SetResource(size_t Index, CTexture* pTexture);

	bool                     SetSampler(CStrID ID, CSampler* pSampler);
	bool                     SetSampler(size_t Index, CSampler* pSampler);

	bool                     Apply();
	void                     Unapply();

	const CShaderParamTable& GetParamTable() const { return *_Table; }
};

}
