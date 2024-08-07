#include "ShaderConstantInfo.h"
#include <algorithm>

namespace Render
{
static const PShaderConstantInfo EmptyInfo;

CShaderConstantInfo::~CShaderConstantInfo() = default;

// TODO: is there better way to clone fields from the base class?
void CShaderConstantInfo::CShaderConstantInfo_CopyFields(const CShaderConstantInfo& Source)
{
	Struct = Source.Struct;
	BufferIndex = Source.BufferIndex;
	Name = Source.Name;
	LocalOffset = Source.LocalOffset;
	ElementStride = Source.ElementStride;
	ElementCount = Source.ElementCount;
	VectorStride = Source.VectorStride;
	ComponentSize = Source.ComponentSize;
	TotalSize = Source.TotalSize;
	Rows = Source.Rows;
	Columns = Source.Columns;
	Flags = Source.Flags;
}
//---------------------------------------------------------------------

const PShaderConstantInfo& CShaderConstantInfo::GetMemberInfo(CStrID Name)
{
	// An array, can't access members even if array element is a structure
	if (ElementCount) return EmptyInfo;

	// Not a structure
	if (!Struct) return EmptyInfo;

	const auto MemberIndex = Struct->FindMemberIndex(Name);

	// Has no member with requested Name
	if (MemberIndex >= Struct->GetMemberCount()) return EmptyInfo;

	// Member cache is not created yet, allocate
	if (!SubInfo) SubInfo = std::make_unique<PShaderConstantInfo[]>(Struct->GetMemberCount());

	// Cache is mapped to struct members, check at member index
	auto& Member = SubInfo[MemberIndex];
	if (!Member)
	{
		Member = Struct->GetMember(MemberIndex)->Clone();
		Member->BufferIndex = BufferIndex;
		Member->CalculateCachedValues();
	}

	return Member;
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetElementInfo()
{
	// Not an array, is element itself 
	if (!ElementCount) return this;

	// If cache is valid, there is the only element there
	if (SubInfo) return SubInfo[0];

	SubInfo = std::make_unique<PShaderConstantInfo[]>(1);
	SubInfo[0] = Clone();
	SubInfo[0]->LocalOffset = 0;
	SubInfo[0]->ElementCount = 0;
	SubInfo[0]->CalculateCachedValues();

	return SubInfo[0];
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetVectorInfo()
{
	// An array, can't access components
	if (ElementCount) return nullptr;

	// Don't check if it is a structure, because structure must have MajorDim = 1
	n_assert_dbg(!Struct);

	// Not a matrix (2-dimensional object), has no vectors
	const auto MajorDim = IsColumnMajor() ? Columns : Rows;
	if (MajorDim < 2) return nullptr;

	// If cache is valid, there are component at [0] and vector at [1]
	if (SubInfo)
	{
		if (SubInfo[1]) return SubInfo[1];
	}
	else
	{
		SubInfo = std::make_unique<PShaderConstantInfo[]>(2);
	}

	SubInfo[1] = Clone();
	if (IsColumnMajor())
	{
		SubInfo[1]->Columns = 1;
		SubInfo[1]->ElementStride = ComponentSize * Rows;
	}
	else
	{
		SubInfo[1]->Rows = 1;
		SubInfo[1]->ElementStride = ComponentSize * Columns;
	}

	SubInfo[1]->CalculateCachedValues();

	return SubInfo[1];
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetComponentInfo()
{
	// An array, can't access components
	if (ElementCount) return nullptr;

	// A structure, can't access components
	if (Struct) return nullptr;

	// Single component constant is a component itself
	if (Rows < 2 && Columns < 2) return this;

	// If cache is valid, there is component at [0]
	if (SubInfo)
	{
		if (SubInfo[0]) return SubInfo[0];
	}
	else
	{
		// If it is a matrix, allocate slot for the row info too
		const auto MajorDim = IsColumnMajor() ? Columns : Rows;
		SubInfo = std::make_unique<PShaderConstantInfo[]>((MajorDim > 1) ? 2 : 1);
	}

	SubInfo[0] = Clone();
	SubInfo[0]->Columns = 1;
	SubInfo[0]->Rows = 1;
	SubInfo[0]->ElementStride = ComponentSize;
	SubInfo[0]->CalculateCachedValues();

	return SubInfo[0];
}
//---------------------------------------------------------------------

void CShaderStructureInfo::SetMembers(std::vector<PShaderConstantInfo>&& Members)
{
	_Members = std::move(Members);
	_Members.shrink_to_fit();
	std::sort(_Members.begin(), _Members.end(), [](const PShaderConstantInfo& a, const PShaderConstantInfo& b)
	{
		return a->Name < b->Name;
	});
}
//---------------------------------------------------------------------

size_t CShaderStructureInfo::FindMemberIndex(CStrID Name) const
{
	auto It = std::lower_bound(_Members.cbegin(), _Members.cend(), Name, [](const PShaderConstantInfo& Member, CStrID Name)
	{
		return Member->Name < Name;
	});
	return (It != _Members.cend() && (*It)->Name == Name) ? std::distance(_Members.cbegin(), It) : _Members.size();
}
//---------------------------------------------------------------------

}
