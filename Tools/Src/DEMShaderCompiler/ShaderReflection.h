#pragma once
#ifndef __DEM_TOOLS_SHADER_REFLECTION_H__
#define __DEM_TOOLS_SHADER_REFLECTION_H__

#include <Data/String.h>
#include <Data/Array.h>
#include <Data/RefCounted.h>

// Data and functions for shader metadata manipulation in both SM3.0 and USM

namespace IO
{
	class CBinaryReader;
	class CBinaryWriter;
};

// Don't change existing values, they are saved to file
enum EShaderModel
{
	ShaderModel_30	= 0,
	ShaderModel_USM	= 1
};

// Don't change existing values, they are saved to file
enum EShaderParamClass
{
	ShaderParam_Const		= 0,
	ShaderParam_Resource	= 1,
	ShaderParam_Sampler		= 2,

	ShaderParam_COUNT,

	ShaderParam_None	// Buffers, structures and other non-param metadata objects
};

enum EShaderConstType
{
	ShaderConst_Bool	= 0,
	ShaderConst_Int,
	ShaderConst_Float,

	ShaderConst_Struct,

	ShaderConst_Invalid
};

enum EShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

class CMetadataObject //???refcounted?
{
public:

	virtual const char*			GetName() const = 0;
	virtual EShaderModel		GetShaderModel() const = 0;
	virtual EShaderParamClass	GetClass() const = 0;
	virtual bool				IsEqual(const CMetadataObject& Other) const = 0;

	bool operator ==(const CMetadataObject& Other) const { return IsEqual(Other); }
	bool operator !=(const CMetadataObject& Other) const { return !IsEqual(Other); }
};

class CShaderMetadata//: public Data::CRefCounted //!!!check internal/external memory mgmt!
{
public:

	virtual bool				Load(IO::CBinaryReader& R) = 0;
	virtual bool				Save(IO::CBinaryWriter& W) const = 0;
	virtual EShaderModel		GetShaderModel() const = 0;
	virtual U32					GetMinFeatureLevel() const = 0;
	virtual void				SetMinFeatureLevel(U32 NewLevel) = 0;
	virtual U64					GetRequiresFlags() const = 0;
	virtual void				SetRequiresFlags(U64 NewFlags) = 0;
	virtual UPTR				GetParamCount(EShaderParamClass Class) const = 0;
	virtual CMetadataObject*	GetParamObject(EShaderParamClass Class, UPTR Index) = 0;
	virtual UPTR				AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject) = 0;
	virtual bool				FindParamObjectByName(EShaderParamClass Class, const char* pName, UPTR& OutIndex) const = 0;
	virtual CMetadataObject*	GetContainingConstantBuffer(CMetadataObject* pMetaObject) = 0;
};

#endif
