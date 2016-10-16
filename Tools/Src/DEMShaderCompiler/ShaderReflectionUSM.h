#pragma once
#ifndef __DEM_TOOLS_SHADER_REFLECTION_USM_H__
#define __DEM_TOOLS_SHADER_REFLECTION_USM_H__

#include <ShaderReflection.h>
#include <Data/Dictionary.h>

// Universal Shader Model (SM4.0 and higher) metadata

struct ID3D11ShaderReflectionType;

// Don't change existing values, they are saved to file
enum EUSMBufferTypeMask
{
	USMBuffer_Texture		= (1 << 30),
	USMBuffer_Structured	= (2 << 30),

	USMBuffer_RegisterMask = ~(USMBuffer_Texture | USMBuffer_Structured)
};

// Don't change existing values, they are saved to file
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change existing values, they are saved to file
enum EUSMResourceType
{
	USMRsrc_Texture1D			= 0,
	USMRsrc_Texture1DArray,
	USMRsrc_Texture2D,
	USMRsrc_Texture2DArray,
	USMRsrc_Texture2DMS,
	USMRsrc_Texture2DMSArray,
	USMRsrc_Texture3D,
	USMRsrc_TextureCUBE,
	USMRsrc_TextureCUBEArray,

	USMRsrc_Unknown
};

class CUSMBufferMeta: public CMetadataObject
{
public:

	CString	Name;
	U32		Register;
	U32		Size;		// For structured buffers - StructureByteStride

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMStructMemberMeta: public CMetadataObject
{
public:

	CString			Name;
	U32				StructIndex;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Flags;			// See EShaderConstFlags

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { FAIL; } // No need
};

class CUSMStructMeta: public CMetadataObject
{
public:

	//CString						Name;
	CArray<CUSMStructMemberMeta>	Members;

	virtual const char*			GetName() const { return NULL; } // Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { FAIL; } // No need
};

// Arrays and mixed-type structs supported
class CUSMConstMeta: public CMetadataObject
{
public:

	CString			Name;
	U32				BufferIndex;
	U32				StructIndex;
	EUSMConstType	Type;
	U32				Offset;
	U32				ElementSize;
	U32				ElementCount;
	U8				Flags;			// See EShaderConstFlags

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Const; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMRsrcMeta: public CMetadataObject
{
public:

	CString				Name;
	EUSMResourceType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Resource; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMSamplerMeta: public CMetadataObject
{
public:

	CString	Name;
	U32		RegisterStart;
	U32		RegisterCount;

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Sampler; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMShaderMeta: public CShaderMetadata
{
private:

	bool			ProcessStructure(ID3D11ShaderReflectionType* pType, U32 StructSize, CDict<ID3D11ShaderReflectionType*, UPTR>& StructCache);

public:

	U32							MinFeatureLevel;
	U64							RequiresFlags;
	CArray<CUSMBufferMeta>		Buffers;
	CArray<CUSMStructMeta>		Structs;
	CArray<CUSMConstMeta>		Consts;
	CArray<CUSMRsrcMeta>		Resources;
	CArray<CUSMSamplerMeta>		Samplers;

	bool						CollectFromBinary(const void* pData, UPTR Size);
	virtual bool				Load(IO::CBinaryReader& R);
	virtual bool				Save(IO::CBinaryWriter& W) const;
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual U32					GetMinFeatureLevel() const { return MinFeatureLevel; }
	virtual U64					GetRequiresFlags() const { return RequiresFlags; }
	virtual UPTR				GetParamCount(EShaderParamClass Class) const;
	virtual CMetadataObject*	GetParamObject(EShaderParamClass Class, UPTR Index);
	virtual CMetadataObject*	GetContainingConstantBuffer(CMetadataObject* pMetaObject);
};

#endif