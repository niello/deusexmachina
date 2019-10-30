#include "SM30ShaderConstant.h"

#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>

namespace Render
{

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
