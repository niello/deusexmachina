#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>

// Internal class that holds constant metadata. Not for use by clients.

namespace Render
{
class CConstantBuffer;
typedef Ptr<class CShaderConstantInfo> PShaderConstantInfo;
typedef Ptr<class CShaderStructureInfo> PShaderStructureInfo;

enum EShaderConstantFlags : U8
{
	ColumnMajor = 0x01
};

class CShaderConstantInfo : public Core::CObject
{
protected:

	// Cached sub-constant info. Any array has elements. Any single structure has members.
	// Any single vector has components. Any single matrix has components at [0] and vectors at [1].
	// Member info is a sorted array mapped to struct members array. It could be a union,
	// but it makes default constructor deleted.
	std::unique_ptr<PShaderConstantInfo[]> SubInfo;

	virtual PShaderConstantInfo Clone() const = 0;
	void                        CShaderConstantInfo_CopyFields(const CShaderConstantInfo& Source);

public:

	PShaderStructureInfo Struct;
	size_t               BufferIndex;
	CStrID               Name;
	U32                  LocalOffset;
	U32                  ElementStride;
	U32                  ElementCount;
	U32                  VectorStride;
	U32                  ComponentSize;
	U8                   Rows;
	U8                   Columns;
	U8                   Flags;

	virtual ~CShaderConstantInfo() override;

	virtual void SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const = 0;
	virtual void SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const = 0;
	virtual void SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const = 0;
	virtual void SetUInts(CConstantBuffer& CB, U32 Offset, const U32* pValue, UPTR Count) const = 0;
	virtual void SetBools(CConstantBuffer& CB, U32 Offset, const bool* pValue, UPTR Count) const = 0;

	CStrID GetID() const { return Name; }
	size_t GetConstantBufferIndex() const { return BufferIndex; }
	U32    GetLocalOffset() const { return LocalOffset; }
	U32    GetElementStride() const { return ElementStride; }
	U32    GetElementCount() const { return ElementCount; }
	U32    GetVectorStride() const { NOT_IMPLEMENTED_MSG("Init value!"); return VectorStride; }
	U32    GetComponentSize() const { NOT_IMPLEMENTED_MSG("Init value!"); return ComponentSize; }
	U32    GetRowCount() const { return Rows; }
	U32    GetColumnCount() const { return Columns; }
	bool   IsColumnMajor() const { return Flags & EShaderConstantFlags::ColumnMajor; }
	bool   HasElementPadding() const;
	bool   NeedConversionFrom(/*type*/) const;

	PShaderConstantInfo GetMemberInfo(CStrID Name);
	PShaderConstantInfo GetElementInfo();
	PShaderConstantInfo GetVectorInfo();
	PShaderConstantInfo GetComponentInfo();
};

class CShaderStructureInfo : public Data::CRefCounted
{
protected:

	//CStrID Name;
	std::vector<PShaderConstantInfo> _Members;

public:

	void                 SetMembers(std::vector<PShaderConstantInfo>&& Members);
	size_t               GetMemberCount() const { return _Members.size(); }
	CShaderConstantInfo* GetMember(size_t Index) { return (Index < _Members.size()) ? _Members[Index].Get() : nullptr; }
	size_t               FindMemberIndex(CStrID Name) const;
};

}
