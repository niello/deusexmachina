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

	U32		Offset;
	HHandle	StructHandle;
	U32		ElementCount;
	U32		ElementSize;
	U8		Columns;
	U8		Rows;
	U8		Flags;

public:

	//CUSMShaderConstant(): Offset(0), StructHandle(INVALID_HANDLE), Columns(0), Rows(0), Flags(0) {}

	virtual bool			Init(HConst hConst);
	virtual UPTR			GetSizeInBytes() const { return ElementCount * ElementSize; }
	virtual UPTR			GetColumnCount() const { return Columns; }
	virtual UPTR			GetRowCount() const { return Rows; }
	virtual UPTR			GetElementCount() const { return ElementCount; }
	virtual UPTR			GetMemberCount() const;
	virtual PShaderConstant	GetElement(U32 Index) const;
	virtual PShaderConstant	GetMember(CStrID Name) const;
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const;
	virtual void			SetUInt(const CConstantBuffer& CB, U32 Value) const;
	virtual void			SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count = 1) const;
	virtual void			SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count = 1, U32 StartIndex = 0) const;
};

typedef Ptr<CUSMShaderConstant> PUSMShaderConstant;

}

#endif
