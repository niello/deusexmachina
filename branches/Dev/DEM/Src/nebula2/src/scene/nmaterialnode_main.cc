//------------------------------------------------------------------------------
//  nmaterialnode_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nmaterialnode.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/nshader2.h"
#include "gfx2/ntexture2.h"
#include "scene/nrendercontext.h"
#include "scene/nsceneserver.h"
#include "kernel/ndebug.h"
#include <Data/BinaryReader.h>

nNebulaClass(nMaterialNode, "ntransformnode");

bool nMaterialNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
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
		default: return nTransformNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

void nMaterialNode::UnloadShader()
{
    if (refShader.isvalid())
    {
        refShader->Release();
        refShader.invalidate();
    }
}
//---------------------------------------------------------------------

bool nMaterialNode::LoadShader()
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

void nMaterialNode::UnloadTexture(int index)
{
	CTextureNode& texNode = texNodeArray[index];
	if (texNode.refTexture.isvalid())
	{
		texNode.refTexture->Release();
		texNode.refTexture.invalidate();
	}
}
//---------------------------------------------------------------------

bool nMaterialNode::LoadTexture(int index)
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

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool nMaterialNode::LoadResources()
{
	if (!nTransformNode::LoadResources()) return false;
	if (!LoadShader()) return false;
	for (int i = 0; i < texNodeArray.Size(); i++)
		if (!LoadTexture(i)) return false;
	return true;
}

//------------------------------------------------------------------------------
/**
    Unload the resources if refcount has reached zero.
*/
void
nMaterialNode::UnloadResources()
{
    nTransformNode::UnloadResources();
    for (int i = 0; i < texNodeArray.Size(); i++)
		UnloadTexture(i);
    UnloadShader();
}

//------------------------------------------------------------------------------
/**
    Setup shader attributes before rendering instances of this scene node.
*/
bool
nMaterialNode::ApplyShader(nSceneServer* sceneServer)
{
    n_assert(sceneServer);

    if (refShader.isvalid())
    {
        // if there are no animators on this node, we only
        // need to set shader params once for the whole instance batch
        //if (GetNumAnimators() == 0)
        //{
            refShader->SetParams(shaderParams);
        //}
        nGfxServer2::Instance()->SetShader(refShader);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Perform per-instance rendering of shader. This should just apply
    shader parameters which may change from instance to instance.
*/
bool
nMaterialNode::RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer && renderContext);

    // FIXME FIXME FIXME
    // THIS IS A LARGE PERFORMANCE BOTTLENECK!
    // NEED TO SPLIT SHADER PARAMETERS INTO STATIC "INSTANCE SET" PARAMETERS
    // AND LIGHTWEIGHT "PER-INSTANCE" PARAMETERS WHICH CAN BE ANIMATED!!!

    // invoke shader manipulators
    //if (GetNumAnimators() > 0)
    //{
	// // Animate shader params here
    //    shader->SetParams(shaderParams);
    //}

    // set shader override parameters from render context (set directly by application)
    refShader->SetParams(renderContext->GetShaderOverrides());

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nMaterialNode::SetTexture(nShaderState::Param param, const char* texName)
{
    n_assert(texName);

    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter in object '%s'\n", this->GetName());
        return;
    }

    // see if texture variable already exists
    int i;
    int num = this->texNodeArray.Size();
    for (i = 0; i < texNodeArray.Size(); i++)
    {
        if (this->texNodeArray[i].shaderParameter == param) break;
    }
    if (i == num)
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

//------------------------------------------------------------------------------
/**
*/
const char* nMaterialNode::GetTexture(nShaderState::Param param) const
{
    for (int i = 0; i < texNodeArray.Size(); i++)
        if (texNodeArray[i].shaderParameter == param)
            return texNodeArray[i].texName.Get();
    return 0;
}
