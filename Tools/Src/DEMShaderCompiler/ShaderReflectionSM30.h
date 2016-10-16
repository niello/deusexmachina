#pragma once
#ifndef __DEM_TOOLS_SHADER_REFLECTION_SM30_H__
#define __DEM_TOOLS_SHADER_REFLECTION_SM30_H__

#include <ShaderReflection.h>

// Legacy Shader Model 3.0 (SM3.0) metadata

// Don't change existing values, they are saved to file
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

// Don't change existing values, they are saved to file
enum ESM30RegisterSet
{
	RS_Bool		= 0,
	RS_Int4		= 1,
	RS_Float4	= 2
};

class CSM30BufferMeta: public CMetadataObject
{
public:

	CString			Name;
	U32				SlotIndex;	// Pseudo-register, two buffers at the same slot will conflict
	CArray<UPTR>	UsedFloat4;
	CArray<UPTR>	UsedInt4;
	CArray<UPTR>	UsedBool;

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CSM30StructMemberMeta: public CMetadataObject
{
public:

	CString			Name;
	U32				StructIndex;
	U32				RegisterOffset;
	U32				ElementRegisterCount;
	U32				ElementCount;
	U8				Flags;					// See EShaderConstFlags
	//???store register set and support mixed structs?

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { FAIL; } // No need
};

class CSM30StructMeta: public CMetadataObject
{
public:

	//CString						Name;
	CArray<CSM30StructMemberMeta>	Members;

	virtual const char*			GetName() const { return NULL; } // Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { FAIL; } // No need
};

// Arrays and single-type structures are supported
class CSM30ConstMeta: public CMetadataObject
{
public:

	CString				Name;
	U32					BufferIndex;
	U32					StructIndex;
	ESM30RegisterSet	RegisterSet;
	U32					RegisterStart;
	U32					ElementRegisterCount;
	U32					ElementCount;
	U8					Flags;					// See EShaderConstFlags

	U32					RegisterCount;			// Cache, not saved

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Const; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
class CSM30RsrcMeta: public CMetadataObject
{
public:

	CString	Name;
	U32		Register;

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Resource; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

// Arrays supported with arbitrarily assigned textures
class CSM30SamplerMeta: public CMetadataObject
{
public:

	CString				Name;
	ESM30SamplerType	Type;
	U32					RegisterStart;
	U32					RegisterCount;

	virtual const char*			GetName() const { return Name.CStr(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Sampler; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CDEMD3DInclude;

class CSM30ShaderMeta: public CShaderMetadata
{
public:

	CArray<CSM30BufferMeta>		Buffers;
	CArray<CSM30StructMeta>		Structs;
	CArray<CSM30ConstMeta>		Consts;
	CArray<CSM30RsrcMeta>		Resources;
	CArray<CSM30SamplerMeta>	Samplers;

	bool						CollectFromBinaryAndSource(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CDEMD3DInclude& IncludeHandler);

	virtual bool				Load(IO::CBinaryReader& R);
	virtual bool				Save(IO::CBinaryWriter& W) const;

	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual U32					GetMinFeatureLevel() const;
	virtual void				SetMinFeatureLevel(U32 NewLevel) { }
	virtual U64					GetRequiresFlags() const { return 0; }
	virtual void				SetRequiresFlags(U64 NewFlags) { }

	virtual UPTR				GetParamCount(EShaderParamClass Class) const;
	virtual CMetadataObject*	GetParamObject(EShaderParamClass Class, UPTR Index);
	virtual UPTR				AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject);
	virtual bool				FindParamObjectByName(EShaderParamClass Class, const char* pName, UPTR& OutIndex) const;

	virtual UPTR				AddOrMergeBuffer(const CMetadataObject* pMetaBuffer);
	virtual CMetadataObject*	GetContainingConstantBuffer(const CMetadataObject* pMetaObject) const;
	virtual bool				SetContainingConstantBuffer(UPTR ConstIdx, UPTR BufferIdx);
};

#endif