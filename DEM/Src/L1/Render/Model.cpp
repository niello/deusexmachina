#include "Model.h"

#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModel, 'MODL', Render::IRenderable);

bool CModel::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'MTRL':
		{
			//Material = RenderSrv->MaterialMgr.GetOrCreateTypedResource(DataReader.Read<CStrID>());
			CStrID URI = DataReader.Read<CStrID>();
			OK;
		}
		case 'VARS':
		{
			U16 Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				//CShaderVar& Var = ShaderVars.Add(VarName);
				//Var.SetName(VarName);
				Data::CData Value = DataReader.Read<Data::CData>();
				//???check type if bound? use SetValue for it?
				//Can set CData type at var creation and set value to it by SetValue, so type will be asserted
				//???WHERE? if (Material.IsValid()) Var.Bind(*Material->GetShader());
			}
			OK;
		}
		case 'TEXS':
		{
			U16 Count;
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
		case 'JPLT':
		{
			U16 Count;
			if (!DataReader.Read(Count)) FAIL;
			BoneIndices.SetSize(Count);
			return DataReader.GetStream().Read(BoneIndices.GetPtr(), Count * sizeof(int)) == Count * sizeof(int);
		}
		case 'MESH':
		{
			//???!!!store the whole URI in a file?!
			//???store RMesh in a CModel? to allow resource validation. Or really no need?
			CString MeshID = DataReader.Read<CString>();
			CStrID MeshURI = CStrID(CString("Meshes:") + MeshID.CStr() + ".nvx2");
			Resources::PResource RMesh = ResourceMgr->RegisterResource(MeshURI);
			if (!RMesh->IsLoaded())
			{
				Resources::PResourceLoader Loader = RMesh->GetLoader();
				if (Loader.IsNullPtr())
					Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CMesh>(PathUtils::GetExtension(MeshURI.CStr()));
				ResourceMgr->LoadResourceSync(*RMesh, *Loader);
				n_assert(RMesh->IsLoaded());
			}
			Mesh = RMesh->GetObject<Render::CMesh>();
			OK;
		}
		case 'MSGR':
		{
			return DataReader.Read(MeshGroupIndex);
		}
		case 'BTYP':
		{
			CStrID BatchType;
			return DataReader.Read(BatchType);
		}
		case 'FFLG':
		{
			CString FeatFlagsStr;
			if (!DataReader.ReadString(FeatFlagsStr)) FAIL;
			//FeatureFlags = RenderSrv->ShaderFeatures.GetMask(FeatFlagsStr);
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

bool CModel::ValidateResources()
{
//	static const CString StrMaterials("Materials:");
//	static const CString StrMeshes("Meshes:");
//	static const CString StrTextures("Textures:");
//
//	if (Material.IsValid())
//	{
//		if (!Material->IsLoaded() && !Render::LoadMaterialFromPRM(StrMaterials + Material->GetUID().CStr() + ".prm", Material)) FAIL;
//
//		for (int i = 0; i < ShaderVars.GetCount(); ++i)
//		{
//			CShaderVar& Var = ShaderVars.ValueAt(i);
//			if (!Var.IsBound()) Var.Bind(*Material->GetShader());
//
//			//!!!non-file textures (forex RTs) will fail to load here! ensure they are
//			// in loaded state or they load themselvef properly!
//			if (Var.Value.IsA<PTexture>())
//			{
//				PTexture Tex = Var.Value.GetValue<PTexture>();
//				if (!Tex->IsLoaded()) LoadTextureUsingD3DX(StrTextures + Tex->GetUID().CStr(), Tex);
//			}
//		}
//	}
//
//	//!!!change extension!
//	if (Mesh.IsValid() && !Mesh->IsLoaded() && !Render::LoadMeshFromNVX2(StrMeshes + Mesh->GetUID().CStr() + ".nvx2", Usage_Immutable, CPU_NoAccess, Mesh)) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CModel::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	n_assert_dbg(Mesh->IsResourceValid());
	const Render::CPrimitiveGroup* pGroup = Mesh->GetGroup(MeshGroupIndex, LOD);
	if (pGroup)
	{
		OutBox = pGroup->AABB;
		OK;
	}
	else FAIL;
}
//---------------------------------------------------------------------

}