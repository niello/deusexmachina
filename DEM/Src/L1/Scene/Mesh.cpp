#include "Mesh.h"

#include <Scene/Scene.h>
#include <Data/BinaryReader.h>

//!!!OLD!
#include "scene/nsceneserver.h"

namespace Scene
{
ImplementRTTI(Scene::CMesh, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CMesh);

bool CMesh::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'RDHS': // SHDR
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetShader(Value);
			OK;
		}
		case 'SRAV': // VARS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				char Key[256];
				if (!DataReader.ReadString(Key, sizeof(Key))) FAIL;
				nShaderState::Param Param = nShaderState::StringToParam(Key);

				char Type;
				if (!DataReader.Read(Type)) FAIL;

				if (Type == DATA_TYPE_ID(bool)) SetBool(Param, DataReader.Read<bool>());
				else if (Type == DATA_TYPE_ID(int)) SetInt(Param, DataReader.Read<int>());
				else if (Type == DATA_TYPE_ID(float)) SetFloat(Param, DataReader.Read<float>());
				else if (Type == DATA_TYPE_ID(vector4)) SetVector(Param, DataReader.Read<vector4>()); //???vector3?
				//else if (Type == DATA_TYPE_ID(matrix44)) SetMatrix(Param, DataReader.Read<matrix44>());
				else FAIL;
			}
			OK;
		}
		case 'SXET': // TEXS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				char Value[512];
				if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
				nShaderState::Param Param = nShaderState::StringToParam(Value);
				if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
				SetTexture(Param, Value);
			}
			OK;
		}
		case 'HSEM': // MESH
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetMesh(Value);
			OK;
		}
		case 'RGSM': // MSGR
		{
			DataReader.Read(groupIndex);
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CMesh::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveElement(pSPSRecord);
		pSPSRecord = NULL;
	}
}
//---------------------------------------------------------------------

void CMesh::Update()
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
void CMesh::GetBox(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	OutBox = refMesh->Group(groupIndex).Box;
	OutBox.transform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

//!!!
// ==================== OLD ===========================================
//!!!

void CMesh::SetTexture(nShaderState::Param param, const char* texName)
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

const char* CMesh::GetTexture(nShaderState::Param param) const
{
    for (int i = 0; i < texNodeArray.Size(); i++)
        if (texNodeArray[i].shaderParameter == param)
            return texNodeArray[i].texName.Get();
    return 0;
}
//---------------------------------------------------------------------

void CMesh::UnloadShader()
{
    if (refShader.isvalid())
    {
        refShader->Release();
        refShader.invalidate();
    }
}
//---------------------------------------------------------------------

bool CMesh::LoadShader()
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

void CMesh::UnloadTexture(int index)
{
	CTextureNode& texNode = texNodeArray[index];
	if (texNode.refTexture.isvalid())
	{
		texNode.refTexture->Release();
		texNode.refTexture.invalidate();
	}
}
//---------------------------------------------------------------------

bool CMesh::LoadTexture(int index)
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

void CMesh::UnloadMesh()
{
	if (refMesh.isvalid())
	{
		refMesh->Release();
		refMesh.invalidate();
	}
}
//---------------------------------------------------------------------

// Load new mesh, release old one if valid. Also initializes the groupIndex member.
bool CMesh::LoadMesh()
{
    if (!refMesh.isvalid() && meshName.IsValid())
    {
        // append mesh usage to mesh resource name
        nString resourceName;
        resourceName.Format("%s_%d", meshName.Get(), meshUsage);

        // get a new or shared mesh
        nMesh2* mesh = nGfxServer2::Instance()->NewMesh(resourceName);
        n_assert(mesh);
        if (!mesh->IsLoaded())
        {
            mesh->SetFilename(meshName);
            mesh->SetUsage(meshUsage);

            if (!mesh->Load())
            {
                n_printf("nMeshNode: Error loading mesh '%s'\n", meshName.Get());
                mesh->Release();
                return false;
            }
        }
        refMesh = mesh;
        //SetLocalBox(refMesh->Group(groupIndex).Box);
    }
    return true;
}
//---------------------------------------------------------------------

bool CMesh::LoadResources()
{
	//if (!nTransformNode::LoadResources()) return false;
	if (!LoadShader()) return false;
	for (int i = 0; i < texNodeArray.Size(); i++)
		if (!LoadTexture(i)) return false;
	if (!LoadMesh()) return false;
	return true;
}
//---------------------------------------------------------------------

void CMesh::UnloadResources()
{
    //nTransformNode::UnloadResources();
    for (int i = 0; i < texNodeArray.Size(); i++)
		UnloadTexture(i);
    UnloadShader();
	UnloadMesh();
}
//---------------------------------------------------------------------

}