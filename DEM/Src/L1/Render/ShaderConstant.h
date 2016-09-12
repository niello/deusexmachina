#pragma once
#ifndef __DEM_L1_RENDER_SHADER_CONSTANT_H__
#define __DEM_L1_RENDER_SHADER_CONSTANT_H__

#include <Data/RefCounted.h> //???or use pool instead?
#include <Render/RenderFwd.h>

// Interface to a shader constant, piece of data on a GPU side. Write-only.
// Doesn't represent a piece of data, but a location inside a constant buffer.

namespace Render
{
typedef Ptr<class CShaderConstant> PShaderConstant;

class CShaderConstant: public Data::CRefCounted
{
public:

	// Pass as the Size to SetRawValue() to specify Size = size of this constant
	static const UPTR WholeSize = 0;

	virtual ~CShaderConstant() {}

	virtual bool			Init(HConst hConst) = 0;
	virtual UPTR			GetElementCount() const = 0;
	virtual UPTR			GetMemberCount() const = 0;
	virtual PShaderConstant	GetElement(U32 Index) const = 0;
	virtual PShaderConstant	GetMember(CStrID Name) const = 0;
	virtual void			SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const = 0;

	//???or separate SetBool, SetFloat(values*, count), SetMatrix[RowMajor](matrix44), SetMatrixColumnMajor() etc
	//template<typename T>
	//void						SetValue(const CConstantBuffer& CB, const T& Value) { return SetRawValue(CB, &Value, sizeof(Value)); }
};

}

#endif
