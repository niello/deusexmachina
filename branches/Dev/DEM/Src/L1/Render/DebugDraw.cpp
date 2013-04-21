#include "DebugDraw.h"

#include <Render/RenderServer.h>

//!!!TMP!
#include <d3dx9.h>

namespace Render
{

bool CDebugDraw::Open()
{
	nArray<CVertexComponent> VC(1, 1);
	CVertexComponent* pCmp = VC.Reserve(1);
	pCmp->Format = CVertexComponent::Float3;
	pCmp->Semantic = CVertexComponent::Position;
	pCmp->Index = 0;
	pCmp->Stream = 0;

	PVertexLayout ShapeVL = RenderSrv->GetVertexLayout(VC);

	pCmp = VC.Reserve(5);
	for (int i = 0; i < 4; ++i)
	{
		pCmp->Format = CVertexComponent::Float4;
		pCmp->Semantic = CVertexComponent::TexCoord;
		pCmp->Index = i + 4; // TEXCOORD 4, 5, 6, 7 are used
		pCmp->Stream = 1;
		++pCmp;
	}

	pCmp->Format = CVertexComponent::Float4;
	pCmp->Semantic = CVertexComponent::Color;
	pCmp->Index = 0;
	pCmp->Stream = 1;

	ShapeInstVL = RenderSrv->GetVertexLayout(VC);

	VC.Erase(0);
	InstVL = RenderSrv->GetVertexLayout(VC);
	InstanceBuffer.Create();
	if (!InstanceBuffer->Create(InstVL, MaxShapesPerDIP, Usage_Dynamic, CPU_Write)) FAIL;

	Shapes = RenderSrv->MeshMgr.GetTypedResource(CStrID("DebugShapes"));
	if (Shapes->IsLoaded()) OK;

	ID3DXMesh* pDXMesh[ShapeCount];
	n_assert(SUCCEEDED(D3DXCreateBox(RenderSrv->GetD3DDevice(), 1.0f, 1.0f, 1.0f, &pDXMesh[0], NULL)));
	n_assert(SUCCEEDED(D3DXCreateSphere(RenderSrv->GetD3DDevice(), 1.0f, 12, 6, &pDXMesh[1], NULL)));
	n_assert(SUCCEEDED(D3DXCreateCylinder(RenderSrv->GetD3DDevice(), 1.0f, 1.0f, 1.0f, 18, 1, &pDXMesh[2], NULL)));

	DWORD VCount = 0, ICount = 0;
	for (int i = 0; i < ShapeCount; ++i)
	{
		VCount += pDXMesh[i]->GetNumVertices();
		ICount += pDXMesh[i]->GetNumFaces();
	}
	ICount *= 3;

	n_assert(VCount < 65536); // 16-bit IB

	PVertexBuffer VB;
	VB.Create();
	if (!VB->Create(ShapeVL, VCount, Usage_Immutable, CPU_NoAccess))
	{
		for (int i = 0; i < ShapeCount; ++i)
			pDXMesh[i]->Release();
		FAIL;
	}

	PIndexBuffer IB;
	IB.Create();
	if (!IB->Create(CIndexBuffer::Index16, ICount, Usage_Immutable, CPU_NoAccess))
	{
		for (int i = 0; i < ShapeCount; ++i)
			pDXMesh[i]->Release();
		FAIL;
	}

	nArray<CMeshGroup> Groups(ShapeCount, 0);

	DWORD VertexOffset = 0, IndexOffset = 0;
	vector3* pVBData = (vector3*)VB->Map(Map_Setup);
	ushort* pIBData = (ushort*)IB->Map(Map_Setup);
	CMeshGroup* pGroup = Groups.Reserve(ShapeCount);
	for (int i = 0; i < ShapeCount; ++i)
	{
		DWORD VertexCount = pDXMesh[i]->GetNumVertices();
		DWORD IndexCount = pDXMesh[i]->GetNumFaces() * 3;

		char* pDXVB;
		n_assert(SUCCEEDED(pDXMesh[i]->LockVertexBuffer(0, (LPVOID*)&pDXVB)));
		for (DWORD j = 0; j < VertexCount; ++j, pDXVB += pDXMesh[i]->GetNumBytesPerVertex())
			*pVBData++ = *(vector3*)pDXVB;
		pDXMesh[i]->UnlockVertexBuffer();

		ushort* pDXIB;
		n_assert(SUCCEEDED(pDXMesh[i]->LockIndexBuffer(0, (LPVOID*)&pDXIB)));
		for (DWORD j = 0; j < IndexCount; ++j, ++pDXIB)
			*pIBData++ = (*pDXIB) + (ushort)VertexOffset;
		pDXMesh[i]->UnlockIndexBuffer();

		pGroup->Topology = TriList;
		pGroup->FirstVertex = VertexOffset;
		pGroup->VertexCount = VertexCount;
		pGroup->FirstIndex = IndexOffset;
		pGroup->IndexCount = IndexCount;
		++pGroup;

		VertexOffset += VertexCount;
		IndexOffset += IndexCount;
	}
	VB->Unmap();
	IB->Unmap();

	for (int i = 0; i < ShapeCount; ++i)
		pDXMesh[i]->Release();

	return Shapes->Setup(VB, IB, Groups);
}
//---------------------------------------------------------------------

void CDebugDraw::Close()
{
	for (int i = 0; i < ShapeCount; ++i)
		ShapeInsts[i].Clear();

	ShapeInstVL = NULL;
	InstVL = NULL;
	Shapes = NULL;
	InstanceBuffer = NULL;
}
//---------------------------------------------------------------------

}