#include "Terrain.h"

#include <IO/BinaryReader.h>
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
			//HeightMap = RenderSrv->TextureMgr.GetOrCreateTypedResource(DataReader.Read<CStrID>());
			CStrID URI = DataReader.Read<CStrID>();
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
		case 'VARS':
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				//CShaderVar& Var = ShaderVars.Add(VarName);
				//Var.SetName(VarName);
				//DataReader.Read(Var.Value);
				Data::CData Value = DataReader.Read<Data::CData>();
			}
			OK;
		}
		case 'TEXS':
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				//CShaderVar& Var = ShaderVars.Add(VarName);
				//Var.SetName(VarName);
				//Var.Value = RenderSrv->TextureMgr.GetOrCreateTypedResource(DataReader.Read<CStrID>());
				CStrID URI = DataReader.Read<CStrID>();
			}
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

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
//	IO::CFileStream CDLODFile;
//	if (!CDLODFile.Open(CString("Terrain:") + HeightMap->GetUID().CStr() + ".cdlod", IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
//	IO::CBinaryReader Reader(CDLODFile);
//
//	n_assert(Reader.Read<int>() == 'CDLD');	// Magic
//	n_assert(Reader.Read<int>() == 1);		// Version
//
//	Reader.Read(HFWidth);
//	Reader.Read(HFHeight);
//	Reader.Read(PatchSize);
//	Reader.Read(LODCount);
//	UPTR MinMaxDataSize = Reader.Read<UPTR>();
//	Reader.Read(VerticalScale);
//	Reader.Read(Box.Min.x);
//	Reader.Read(Box.Min.y);
//	Reader.Read(Box.Min.z);
//	Reader.Read(Box.Max.x);
//	Reader.Read(Box.Max.y);
//	Reader.Read(Box.Max.z);
//
//	if (!HeightMap->IsLoaded())
//	{
//		//!!!write R32F variant!
//		n_assert(RenderSrv->CheckCaps(Render::Caps_VSTex_L16));
//
//		if (!HeightMap->Create(Render::CTexture::Texture2D, D3DFMT_L16, HFWidth, HFHeight, 0, 1, Render::Usage_Immutable, Render::CPU_NoAccess))
//			FAIL;
//
//		Render::CTexture::CMapInfo MapInfo;
//		if (!HeightMap->Map(0, Map_Setup, MapInfo)) FAIL;
//		CDLODFile.Read(MapInfo.pData, HFWidth * HFHeight * sizeof(unsigned short));
//		HeightMap->Unmap(0);
//	}
//	else CDLODFile.Seek(HFWidth * HFHeight * sizeof(unsigned short), IO::Seek_Current);
//
//	pMinMaxData = (short*)n_malloc(MinMaxDataSize);
//	CDLODFile.Read(pMinMaxData, MinMaxDataSize);
//
//	//???store dimensions along with a pointer?
//	UPTR PatchesW = (HFWidth - 1 + PatchSize - 1) / PatchSize;
//	UPTR PatchesH = (HFHeight - 1 + PatchSize - 1) / PatchSize;
//	UPTR Offset = 0;
//	CMinMaxMap* pMMMap = MinMaxMaps.Reserve(LODCount);
//	for (UPTR LOD = 0; LOD < LODCount; ++LOD, ++pMMMap)
//	{
//		pMMMap->PatchesW = PatchesW;
//		pMMMap->PatchesH = PatchesH;
//		pMMMap->pData = pMinMaxData + Offset;
//		Offset += PatchesW * PatchesH * 2;
//		PatchesW = (PatchesW + 1) / 2;
//		PatchesH = (PatchesH + 1) / 2;
//	}
//
//	UPTR TopPatchSize = PatchSize << (LODCount - 1);
//	TopPatchCountX = (HFWidth - 1 + TopPatchSize - 1) / TopPatchSize;
//	TopPatchCountZ = (HFHeight - 1 + TopPatchSize - 1) / TopPatchSize;
//
//	static const CString StrTextures("Textures:");
//
//	for (int i = 0; i < ShaderVars.GetCount(); ++i)
//	{
//		CShaderVar& Var = ShaderVars.ValueAt(i);
//		if (Var.Value.IsA<PTexture>())
//		{
//			PTexture Tex = Var.Value.GetValue<PTexture>();
//			if (!Tex->IsLoaded()) LoadTextureUsingD3DX(StrTextures + Tex->GetUID().CStr(), Tex);
//		}
//	}
//
//	PatchMesh = GetPatchMesh(PatchSize);
//	QuarterPatchMesh = GetPatchMesh(PatchSize >> 1);

	OK;
}
//---------------------------------------------------------------------

}