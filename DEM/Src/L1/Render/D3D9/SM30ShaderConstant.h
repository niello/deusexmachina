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
	U32					SizeInBytes;
	U8					Columns;
	U8					Rows;
	U8					Flags;

public:

	CSM30ShaderConstant(): RegSet(Reg_Invalid) {}

	virtual bool			Init(HConst hConst);
	virtual UPTR			GetSizeInBytes() const { return SizeInBytes; }
	virtual UPTR			GetElementCount() const { return ElementCount; }
	virtual UPTR			GetMemberCount() const;
	virtual PShaderConstant	GetElement(U32 Index) const;
	virtual PShaderConstant	GetMember(CStrID Name) const;
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const;
	virtual void			SetUInt(const CConstantBuffer& CB, U32 Value) const;
	virtual void			SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count = 1) const;
	virtual void			SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count = 1, U32 StartIndex = 0) const;
};

typedef Ptr<CSM30ShaderConstant> PSM30ShaderConstant;

}

#endif
