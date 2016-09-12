#pragma once
#ifndef __DEM_L1_RENDER_USM_SHADER_CONSTANT_H__
#define __DEM_L1_RENDER_USM_SHADER_CONSTANT_H__

#include <Render/ShaderConstant.h>

// Unified Shader Model (4.0+) constant implementation

namespace Render
{

class CUSMShaderConstant: public CShaderConstant
{
protected:

	U32					Offset;
	HHandle				StructHandle;
	U32					ElementCount;
	U32					ElementSize;

public:

	CUSMShaderConstant()/*: RegSet(Reg_Invalid)*/ {}

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

typedef Ptr<CUSMShaderConstant> PUSMShaderConstant;

}

#endif
