#include "Model.h"

#include <Scene/SPS.h>
#include <Scene/SceneNode.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CModel, 'MODL', Frame::CRenderObject);

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
			short Count;
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
		case 'JPLT':
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			BoneIndices.SetSize(Count);
			return DataReader.GetStream().Read(BoneIndices.GetPtr(), Count * sizeof(int)) == Count * sizeof(int);
		}
		case 'MESH':
		{
			CStrID MeshID = DataReader.Read<CStrID>();
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
			OK;
		}
		case 'MSGR':
		{
			return DataReader.Read(MeshGroupIndex);
		}
		case 'BTYP':
		{
			return DataReader.Read(BatchType);
		}
		case 'FFLG':
		{
			CString FeatFlagsStr;
			if (!DataReader.ReadString(FeatFlagsStr)) FAIL;
			//FeatureFlags = RenderSrv->ShaderFeatures.GetMask(FeatFlagsStr);
			OK;
		}
		default: return CNodeAttribute::LoadDataBlock(FourCC, DataReader);
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

// Now resources are shared and aren't unloaded
// If it is necessary to unload resources (decrement refcount), resource IDs must be saved,
// so pointers can be cleared, but model is able to reload resources from IDs
void CModel::OnDetachFromNode()
{
//	//???do it on deactivation of an attribute? even it is not detached from node
	if (pSPSRecord)
	{
		pSPS->RemoveRecord(pSPSRecord);
		pSPSRecord = NULL;
		pSPS = NULL;
	}
	CRenderObject::OnDetachFromNode();
}
//---------------------------------------------------------------------

void CModel::UpdateInSPS(Scene::CSPS& SPS)
{
	if (pSPS != &SPS)
	{
		if (pSPS)
		{
			if (pSPSRecord)
			{
				pSPS->RemoveRecord(pSPSRecord);
				pSPSRecord = NULL;
			}
			else pSPS->OversizedObjects.RemoveByValue(this);
		}

		pSPS = &SPS;
	}

	if (!pSPSRecord)
	{
		CAABB Box;
		GetGlobalAABB(pSPSRecord->GlobalBox); //???!!!LOD?! //???calc cached and reuse here?
		pSPSRecord = SPS.AddRecord(Box, this);
	}
	else if (pNode->IsWorldMatrixChanged()) //!!! || Group.LocalBox changed
	{
		GetGlobalAABB(pSPSRecord->GlobalBox); //???!!!LOD?!
		SPS.UpdateRecord(pSPSRecord);
		Flags.Clear(WorldMatrixChanged);
	}
}
//---------------------------------------------------------------------

const CAABB& CModel::GetLocalAABB(UPTR LOD) const
{
	const Render::CPrimitiveGroup* pGroup = Mesh->GetGroup(MeshGroupIndex, LOD);
	return pGroup ? pGroup->AABB : CAABB::Empty;
}
//---------------------------------------------------------------------

//!!!differ between CalcBox - primary source, and GetGlobalAABB - return cached box from spatial record!
//???inline?
void CModel::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	n_assert_dbg(Mesh->IsResourceValid());
	const Render::CPrimitiveGroup* pGroup = Mesh->GetGroup(MeshGroupIndex, LOD);
	if (pGroup)
	{
		OutBox = pGroup->AABB;
		OutBox.Transform(pNode->GetWorldMatrix());
	}
	else OutBox = CAABB::Empty;
}
//---------------------------------------------------------------------

}