#include "Model.h"

#include <Scene/Scene.h>
#include <Render/Renderer.h>
#include <Data/BinaryReader.h>

//!!!OLD!
#include "scene/nsceneserver.h"

namespace Render
{
	bool LoadMaterialFromPRM(const nString& FileName, PMaterial OutMaterial);
	bool LoadMeshFromNVX2(const nString& FileName, PMesh OutMesh);
}

namespace Scene
{
ImplementRTTI(Scene::CModel, Scene::CSceneNodeAttr);
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
				//if (Material.isvalid() && Material->IsLoaded()) Var.Bind(*Material->GetShader());
				DataReader.Read(Var.Value);
				//???check type if bound? use SetValue for it?
				//Can set CData type at var creation and set value to it by SetValue, so type will be asserted
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
				if (Material.isvalid()) Var.Bind(*Material->GetShader());

				CStrID TextureID;
				DataReader.Read(TextureID);

				SetTexture(nShaderState::StringToParam(VarName.CStr()), TextureID.CStr());

				Var.Value = (Render::PTexture)RenderSrv->TextureMgr.GetTypedResource(TextureID);
			}
			OK;
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
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CModel::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveElement(pSPSRecord);
		pSPSRecord = NULL;
	}
}
//---------------------------------------------------------------------

void CModel::Update()
{
	if (!pSPSRecord)
	{
		CSPSRecord NewRec(*this);
		GetBox(NewRec.GlobalBox);
		pSPSRecord = pNode->GetScene()->SPS.AddObject(NewRec);
	}
	else if (pNode->IsWorldMatrixChanged()) //!!! || Group.LocalBox changed
	{
		GetBox(pSPSRecord->GlobalBox);
		pNode->GetScene()->SPS.UpdateElement(pSPSRecord);
	}
}
//---------------------------------------------------------------------

//!!!differ between CalcBox - primary source, and GetBox - return cached box from spatial record!
//???inline?
void CModel::GetBox(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	OutBox = Mesh->GetGroup(MeshGroupIndex).AABB;
	OutBox.transform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

//!!!
// ==================== OLD ===========================================
//!!!

void CModel::SetTexture(nShaderState::Param param, const char* texName)
{
    n_assert(texName);

    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter\n");
        return;
    }

    // see if texture variable already exists
    int i;
    for (i = 0; i < texNodeArray.Size(); i++)
        if (this->texNodeArray[i].shaderParameter == param) break;
    if (i == texNodeArray.Size())
    {
        // add new texnode to array
        CTextureNode newTexNode(param, texName);
        this->texNodeArray.Append(newTexNode);
    }
    else
    {
        // invalidate existing texture
        this->UnloadTexture(i);
        this->texNodeArray[i].texName = texName;
    }
    // flag to load resources
    this->resourcesValid = false;
}
//---------------------------------------------------------------------

const char* CModel::GetTexture(nShaderState::Param param) const
{
    for (int i = 0; i < texNodeArray.Size(); i++)
        if (texNodeArray[i].shaderParameter == param)
            return texNodeArray[i].texName.Get();
    return 0;
}
//---------------------------------------------------------------------

void CModel::UnloadShader()
{
    if (refShader.isvalid())
    {
        refShader->Release();
        refShader.invalidate();
    }
}
//---------------------------------------------------------------------

bool CModel::LoadShader()
{
    n_assert(!shaderName.IsEmpty());

    if (!refShader.isvalid())
    {
        const CFrameShader* pFrameShader = nSceneServer::Instance()->GetRenderPath();
        n_assert(pFrameShader);
        int RPShaderIdx = pFrameShader->FindShaderIndex(shaderName);
        n_assert(-1 != RPShaderIdx);
        const nRpShader& rpShader = pFrameShader->shaders[RPShaderIdx];
        shaderIndex = rpShader.GetBucketIndex();
        refShader = rpShader.GetShader();
        refShader->AddRef();
    }
    return true;
}
//---------------------------------------------------------------------

void CModel::UnloadTexture(int index)
{
	CTextureNode& texNode = texNodeArray[index];
	if (texNode.refTexture.isvalid())
	{
		texNode.refTexture->Release();
		texNode.refTexture.invalidate();
	}
}
//---------------------------------------------------------------------

bool CModel::LoadTexture(int index)
{
    CTextureNode& texNode = texNodeArray[index];
    if ((!texNode.refTexture.isvalid()) && (!texNode.texName.IsEmpty()))
    {
        // load only if the texture is used in the shader
        if (IsTextureUsed(texNode.shaderParameter))
        {
            nTexture2* tex = nGfxServer2::Instance()->NewTexture(texNode.texName);
            n_assert(tex);
            if (!tex->IsLoaded())
            {
                tex->SetFilename(texNode.texName);
                if (!tex->Load())
                {
                    n_printf("nMaterialNode: Error loading texture '%s'\n", texNode.texName.Get());
                    return false;
                }
            }
            texNode.refTexture = tex;
            shaderParams.SetArg(texNode.shaderParameter, nShaderArg(tex));
        }
    }
    return true;
}
//---------------------------------------------------------------------

bool CModel::LoadResources()
{
	//!!!TMP! write more elegant! Hide LoadSmthFromFMT calls somewhere in Loaders or smth.

	if (!Material->IsLoaded() && !Render::LoadMaterialFromPRM(Material->GetUID().CStr(), Material)) FAIL;

	// Load local textures (static textures must be loaded when material is loaded?)
	for (int i = 0; i < texNodeArray.Size(); i++)
		if (!LoadTexture(i)) FAIL;

	if (!Mesh->IsLoaded() && !Render::LoadMeshFromNVX2(Mesh->GetUID().CStr(), Mesh)) FAIL; //!!!usage & access!

	OK;
}
//---------------------------------------------------------------------

void CModel::UnloadResources()
{
	for (int i = 0; i < texNodeArray.Size(); i++)
		UnloadTexture(i);

	// Now resources are shared and aren't unloaded
	// If it is necessary to unload resources (decrement refcount), resource IDs must be saved,
	// so pointers can be cleared, but model is able to reload resources from IDs
}
//---------------------------------------------------------------------

}