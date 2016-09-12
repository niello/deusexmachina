#pragma once
#ifndef __DEM_L1_RENDER_SM30_SHADER_CONSTANT_H__
#define __DEM_L1_RENDER_SM30_SHADER_CONSTANT_H__

#include <Render/ShaderConstant.h>
#include <Render/D3D9/D3D9Fwd.h>

// Shader Model 3.0 constant implementation

namespace Render
{

class CSM30ShaderConstant: public CShaderConstant
{
protected:

	U32					Offset;
	ESM30RegisterSet	RegSet;
	HHandle				StructHandle;
	U32					ElementCount;
	U32					ElementRegisterCount;

public:

	CSM30ShaderConstant(): RegSet(Reg_Invalid) {}

	//virtual ~CSM30ShaderConstant() {}

	virtual bool			Init(HConst hConst);
	virtual UPTR			GetElementCount() const { return ElementCount; }
	virtual UPTR			GetMemberCount() const;
	virtual PShaderConstant	GetElement(U32 Index) const;
	virtual PShaderConstant	GetMember(CStrID Name) const;
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const;

	//???or separate SetBool, SetFloat(values*, count), SetMatrix[RowMajor](matrix44), SetMatrixColumnMajor() etc
	//template<typename T>
	//void						SetValue(const CConstantBuffer& CB, const T& Value) { return SetRawValue(CB, &Value, sizeof(Value)); }
};

typedef Ptr<CSM30ShaderConstant> PSM30ShaderConstant;

}

#endif
