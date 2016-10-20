#pragma once
#ifndef __DEM_L1_RENDER_SHADER_CONSTANT_H__
#define __DEM_L1_RENDER_SHADER_CONSTANT_H__

#include <Data/RefCounted.h> //???or use pool instead?
#include <Render/RenderFwd.h>

// Interface to a shader constant, piece of data on a GPU side. Write-only.
// Doesn't represent a piece of data, but a location inside a constant buffer.

class matrix44;

namespace Render
{
typedef Ptr<class CShaderConstant> PShaderConstant;

class CShaderConstant: public Data::CRefCounted
{
protected:

	HConstBuffer BufferHandle;

public:

	// Pass as the Size to SetRawValue() to specify Size = size of this constant
	static const UPTR WholeSize = 0;

	CShaderConstant(): BufferHandle(INVALID_HANDLE) {}
	virtual ~CShaderConstant() {}

	virtual bool			Init(HConst hConst) = 0;

	virtual UPTR			GetSizeInBytes() const = 0;
	virtual UPTR			GetColumnCount() const = 0;
	virtual UPTR			GetRowCount() const = 0;
	virtual UPTR			GetElementCount() const = 0;
	virtual UPTR			GetMemberCount() const = 0;
	virtual PShaderConstant	GetElement(U32 Index) const = 0;
	virtual PShaderConstant	GetMember(CStrID Name) const = 0;
	HConstBuffer			GetConstantBufferHandle() const { return BufferHandle; }

	//!!!???add SetBool, SetInt, SetUInt, SetFloat for single value with pass-by-value?!
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const = 0;
	//virtual void			SetBool(const CConstantBuffer& CB, const bool* pValues, UPTR Count = 1) const = 0;
	//virtual void			SetBool(const CConstantBuffer& CB, const U32* pValues, UPTR Count = 1) const = 0;
	//virtual void			SetInt(const CConstantBuffer& CB, const I32* pValues, UPTR Count = 1) const = 0;
	virtual void			SetUInt(const CConstantBuffer& CB, U32 Value) const = 0;
	//virtual void			SetUInt(const CConstantBuffer& CB, const U32* pValues, UPTR Count = 1) const = 0;
	virtual void			SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count = 1) const = 0;
	virtual void			SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count = 1, U32 StartIndex = 0) const = 0;
};

}

#endif
