#pragma once
#ifndef __DEM_L1_RENDER_SM30_SHADER_CONSTANT_H__
#define __DEM_L1_RENDER_SM30_SHADER_CONSTANT_H__

#include <Render/ShaderConstant.h>

// Shader Model 3.0 constant implementation

namespace Render
{
class CD3D9ConstantBuffer;

class CSM30Constant: public CShaderConstant
{
protected:

	HHandle				StructHandle;
	ESM30RegisterSet	RegSet;
	U32					Offset;
	U32					ElementCount;
	U32					ElementRegisterCount;
	U32					SizeInBytes;
	U8					Columns;
	U8					Rows;
	U8					Flags;

	U32						GetComponentOffset(U32 ComponentIndex) const;
	void					InternalSetUInt(CD3D9ConstantBuffer& CB9, U32 ValueOffset, U32 Value) const;
	void					InternalSetSInt(CD3D9ConstantBuffer& CB9, U32 ValueOffset, I32 Value) const;

public:

	CSM30Constant(): RegSet(Reg_Invalid) {}

	virtual bool			Init(HConstant hConst);
	virtual UPTR			GetSizeInBytes() const { return SizeInBytes; }
	virtual UPTR			GetColumnCount() const { return Columns; }
	virtual UPTR			GetRowCount() const { return Rows; }
	virtual UPTR			GetElementCount() const { return ElementCount; }
	virtual UPTR			GetMemberCount() const;
	virtual PShaderConstant	GetElement(U32 Index) const;
	virtual PShaderConstant	GetMember(CStrID Name) const;
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const;
	virtual void			SetUInt(const CConstantBuffer& CB, U32 Value) const;
	virtual void			SetUIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, U32 Value) const;
	virtual void			SetSInt(const CConstantBuffer& CB, I32 Value) const;
	virtual void			SetSIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, I32 Value) const;
	virtual void			SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count = 1) const;
	virtual void			SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count = 1, U32 StartIndex = 0) const;
};

typedef Ptr<CSM30Constant> PSM30Constant;

}

#endif
