#include "SM30ShaderConstant.h"

#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>

namespace Render
{

//???process column-major differently?
U32 CSM30Constant::GetComponentOffset(U32 ComponentIndex) const
{
	n_assert_dbg(StructHandle == INVALID_HANDLE);

	const U32 ComponentsPerElement = Columns * Rows;
	const U32 Elm = ComponentIndex / ComponentsPerElement;
	ComponentIndex = ComponentIndex - Elm * ComponentsPerElement;
	const U32 Row = ComponentIndex / Columns;
	const U32 Col = ComponentIndex - Row * Columns;

	const U32 ComponentSize = 4; // Always 32-bit, even bool
	const U32 ComponentsPerAlignedRow = 4; // Even for, say, float3x3, each row uses full 4-component register

	return Offset + Elm * ComponentsPerElement + Row * ComponentsPerAlignedRow + Col; // In register components
}
//---------------------------------------------------------------------

UPTR CSM30Constant::GetMemberCount() const
{
	if (StructHandle == INVALID_HANDLE) return 0;
	CSM30StructMeta* pStructMeta = (CSM30StructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	return pStructMeta ? pStructMeta->Members.GetCount() : 0;
}
//---------------------------------------------------------------------

PShaderConstant CSM30Constant::GetElement(U32 Index) const
{
	if (Index >= ElementCount) return nullptr;

	//!!!can use pool!
	PSM30Constant Const = n_new(CSM30Constant);
	Const->Offset = Offset + Index * ElementRegisterCount;
	Const->RegSet = RegSet;
	Const->StructHandle = StructHandle;
	Const->ElementCount = 1;
	Const->ElementRegisterCount = ElementRegisterCount;
	Const->Columns = Columns;
	Const->Rows = Rows;
	Const->Flags = Flags;

	return Const.Get();
}
//---------------------------------------------------------------------

PShaderConstant CSM30Constant::GetMember(CStrID Name) const
{
	CSM30StructMeta* pStructMeta = (CSM30StructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return nullptr;

	//???sort?
	for (CFixedArray<CSM30StructMemberMeta>::CIterator It = pStructMeta->Members.Begin(); It < pStructMeta->Members.End(); ++It)
		if (It->Name == Name)
		{
			//!!!can use pool!
			PSM30Constant Const = n_new(CSM30Constant);
			Const->Offset = Offset + It->RegisterOffset;
			Const->RegSet = RegSet;
			Const->StructHandle = It->StructHandle;
			Const->ElementCount = It->ElementCount;
			Const->ElementRegisterCount = It->ElementRegisterCount;
			Const->Columns = It->Columns;
			Const->Rows = It->Rows;
			Const->Flags = It->Flags;

			return Const.Get();
		}

	return nullptr;
}
//---------------------------------------------------------------------

void CSM30Constant::SetUIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, U32 Value) const
{
	if (StructHandle != INVALID_HANDLE) return;
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;
	InternalSetUInt(CB9, GetComponentOffset(ComponentIndex), Value);
}
//---------------------------------------------------------------------

void CSM30Constant::SetSIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, I32 Value) const
{
	if (StructHandle != INVALID_HANDLE) return;
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;
	InternalSetSInt(CB9, GetComponentOffset(ComponentIndex), Value);
}
//---------------------------------------------------------------------

}
