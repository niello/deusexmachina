#include "Terrain.h"

#include <Render/Material.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrain, 'TERR', Render::IRenderable);

bool CTerrain::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'CDLD':
		{
			CString RsrcURI("Terrain:");
			RsrcURI += DataReader.Read<CStrID>().CStr();
			RsrcURI += ".cdlod";
			RCDLODData = ResourceMgr->RegisterResource(RsrcURI);
			OK;
		}
		case 'MTRL':
		{
			CString RsrcID = DataReader.Read<CString>();
			CStrID RsrcURI = CStrID(CString("Materials:") + RsrcID.CStr() + ".mtl"); //???replace ID by full URI on export?
			RMaterial = ResourceMgr->RegisterResource(RsrcURI);
			OK;
		}
		case 'TSSX':
		{
			InvSplatSizeX = 1.f / DataReader.Read<float>();
			OK;
		}
		case 'TSSZ':
		{
			InvSplatSizeZ = 1.f / DataReader.Read<float>();
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

//???here?
//CMesh* CTerrain::GetPatchMesh(UPTR Size)
//{
//	if (!IsPow2(Size) || Size < 2) return NULL;
//
//	CString PatchName;
//	PatchName.Format("Patch%dx%d", Size, Size);
//	PMesh Patch = RenderSrv->MeshMgr.GetOrCreateTypedResource(CStrID(PatchName.CStr()));
//
//	if (!Patch->IsLoaded())
//	{
//		float InvEdgeSize = 1.f / (float)Size;
//		UPTR VerticesPerEdge = Size + 1;
//		UPTR VertexCount = VerticesPerEdge * VerticesPerEdge;
//		n_assert(VertexCount <= 65535); // because of 16-bit index buffer
//
//		CArray<CVertexComponent> PatchVC;
//		CVertexComponent& Cmp = *PatchVC.Reserve(1);
//		Cmp.Format = CVertexComponent::Float2;
//		Cmp.Semantic = CVertexComponent::Position;
//		Cmp.Index = 0;
//		Cmp.Stream = 0;
//		PVertexLayout PatchVertexLayout = RenderSrv->GetVertexLayout(PatchVC);
//
//		PVertexBuffer VB = n_new(CVertexBuffer);
//		n_assert(VB->Create(PatchVertexLayout, VertexCount, Usage_Immutable, CPU_NoAccess));
//		vector2* pVBData = (vector2*)VB->Map(Map_Setup);
//		for (UPTR z = 0; z < VerticesPerEdge; ++z)
//			for (UPTR x = 0; x < VerticesPerEdge; ++x)
//				pVBData[z * VerticesPerEdge + x].set(x * InvEdgeSize, z * InvEdgeSize);
//		VB->Unmap();
//
//		//???use TriStrip?
//		UPTR IndexCount = Size * Size * 6;
//
//		PIndexBuffer IB = n_new(CIndexBuffer);
//		n_assert(IB->Create(CIndexBuffer::Index16, IndexCount, Usage_Immutable, CPU_NoAccess));
//		ushort* pIBData = (ushort*)IB->Map(Map_Setup);
//		for (UPTR z = 0; z < Size; ++z)
//			for (UPTR x = 0; x < Size; ++x)
//			{
//				*pIBData++ = (ushort)(z * VerticesPerEdge + x);
//				*pIBData++ = (ushort)(z * VerticesPerEdge + (x + 1));
//				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + x);
//				*pIBData++ = (ushort)(z * VerticesPerEdge + (x + 1));
//				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + (x + 1));
//				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + x);
//			}
//		IB->Unmap();
//
//		CArray<CPrimitiveGroup> MeshGroups(1, 0);
//		CPrimitiveGroup& Group = *MeshGroups.Reserve(1);
//		Group.Topology = TriList;
//		Group.FirstVertex = 0;
//		Group.VertexCount = VertexCount;
//		Group.FirstIndex = 0;
//		Group.IndexCount = IndexCount;
//		Group.AABB.Min = vector3::Zero;
//		Group.AABB.Max.set(1.f, 0.f, 1.f);
//
//		n_assert(Patch->Setup(VB, IB, MeshGroups));
//	}
//
//	return Patch;
//}
////---------------------------------------------------------------------

bool CTerrain::ValidateResources()
{
	if (!RCDLODData->IsLoaded())
	{
		Resources::PResourceLoader Loader = RCDLODData->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CCDLODData>(PathUtils::GetExtension(RCDLODData->GetUID()));
		ResourceMgr->LoadResourceSync(*RCDLODData, *Loader);
		if (!RCDLODData->IsLoaded()) FAIL;
	}
	CDLODData = RCDLODData->GetObject<Render::CCDLODData>();

	//!!!if CDLOD will not include texture, just height data, create texture here, if not created!

	if (!RMaterial->IsLoaded())
	{
		Resources::PResourceLoader Loader = RMaterial->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMaterial>(PathUtils::GetExtension(RMaterial->GetUID()));
		ResourceMgr->LoadResourceSync(*RMaterial, *Loader);
		n_assert(RMaterial->IsLoaded());
	}
	Material = RMaterial->GetObject<Render::CMaterial>();

	//	PatchMesh = GetPatchMesh(PatchSize);
	//	QuarterPatchMesh = GetPatchMesh(PatchSize >> 1);

	OK;
}
//---------------------------------------------------------------------

}