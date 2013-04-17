#include "Model.h"

#include <Scene/Scene.h>
#include <Render/RenderServer.h>
#include <Data/BinaryReader.h>

namespace Render
{
	bool LoadMaterialFromPRM(const nString& FileName, PMaterial OutMaterial);
	bool LoadTextureUsingD3DX(const nString& FileName, PTexture OutTexture);
	bool LoadMeshFromNVX2(const nString& FileName, PMesh OutMesh);
}

namespace Scene
{
ImplementRTTI(Scene::CModel, Scene::CRenderObject);
ImplementFactory(Scene::CModel);

bool CModel::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'LRTM': // MTRL
		{
			Material = RenderSrv->MaterialMgr.GetTypedResource(DataReader.Read<CStrID>());
			OK;
		}
		case 'SRAV': // VARS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				Var.SetName(VarName);
				DataReader.Read(Var.Value);
				//???check type if bound? use SetValue for it?
				//Can set CData type at var creation and set value to it by SetValue, so type will be asserted
				//???WHERE? if (Material.isvalid()) Var.Bind(*Material->GetShader());
			}
			OK;
		}
		case 'SXET': // TEXS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				Var.SetName(VarName);
				Var.Value = RenderSrv->TextureMgr.GetTypedResource(DataReader.Read<CStrID>());
			}
			OK;
		}
		case 'TLPJ': // JPLT
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			BoneIndices.SetSize(Count);
			return DataReader.GetStream().Read(BoneIndices.GetPtr(), Count * sizeof(int)) == Count * sizeof(int);
		}
		case 'HSEM': // MESH
		{
			Mesh = RenderSrv->MeshMgr.GetTypedResource(DataReader.Read<CStrID>());
			OK;
		}
		case 'RGSM': // MSGR
		{
			return DataReader.Read(MeshGroupIndex);
		}
		case 'PYTB': // BTYP
		{
			return DataReader.Read(BatchType);
		}
		case 'GLFF': // FFLG
		{
			nString FeatFlagsStr;
			if (!DataReader.ReadString(FeatFlagsStr)) FAIL;
			FeatureFlags = RenderSrv->ShaderFeatureStringToMask(FeatFlagsStr);
			OK;
		}
		default: return CSceneNodeAttr::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

bool CModel::OnAdd()
{
	//!!!TMP! write more elegant! Hide LoadSmthFromFMT calls somewhere in Loaders or smth.

	if (Material.isvalid())
	{
		if (!Material->IsLoaded() && !Render::LoadMaterialFromPRM(Material->GetUID().CStr(), Material)) FAIL;

		for (int i = 0; i < ShaderVars.Size(); ++i)
		{
			CShaderVar& Var = ShaderVars.ValueAtIndex(i);
			Var.Bind(*Material->GetShader());

			//!!!non-file textures (forex RTs) will fail to load here! ensure they are
			// in loaded state or they load themselvef properly!
			if (Var.Value.IsA<PTexture>())
			{
				PTexture Tex = Var.Value.GetValue<PTexture>();
				if (!Tex->IsLoaded()) LoadTextureUsingD3DX(Tex->GetUID().CStr(), Tex);
			}
		}
	}

	//!!!usage & access! //???set in mesh before loading?
	if (Mesh.isvalid() && !Mesh->IsLoaded() && !Render::LoadMeshFromNVX2(Mesh->GetUID().CStr(), Mesh)) FAIL;

	OK;
}
//---------------------------------------------------------------------

// Now resources are shared and aren't unloaded
// If it is necessary to unload resources (decrement refcount), resource IDs must be saved,
// so pointers can be cleared, but model is able to reload resources from IDs
void CModel::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveObject(pSPSRecord);
		n_delete(pSPSRecord);
		pSPSRecord = NULL;
	}
}
//---------------------------------------------------------------------

void CModel::Update()
{
	if (!pSPSRecord)
	{
		pSPSRecord = n_new(CSPSRecord)(*this);
		GetGlobalAABB(pSPSRecord->GlobalBox);
		pNode->GetScene()->SPS.AddObject(pSPSRecord);
	}
	else if (pNode->IsWorldMatrixChanged()) //!!! || Group.LocalBox changed
	{
		GetGlobalAABB(pSPSRecord->GlobalBox);
		pNode->GetScene()->SPS.UpdateObject(pSPSRecord);
	}
}
//---------------------------------------------------------------------

//!!!differ between CalcBox - primary source, and GetGlobalAABB - return cached box from spatial record!
//???inline?
void CModel::GetGlobalAABB(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	OutBox = Mesh->GetGroup(MeshGroupIndex).AABB;
	OutBox.transform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}