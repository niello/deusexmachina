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
#include "scene/nanimator.h"
#include <Data/BinaryReader.h>

nNebulaClass(nMaterialNode, "nabstractshadernode");

//------------------------------------------------------------------------------
/**
*/
nMaterialNode::nMaterialNode() :
    shaderIndex(-1)
{
    // empty
}

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
		default: return nAbstractShaderNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Unload all shaders.
*/
void
nMaterialNode::UnloadShader()
{
    if (this->refShader.isvalid())
    {
        this->refShader->Release();
        this->refShader.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Load shader resources.
*/
bool
nMaterialNode::LoadShader()
{
    n_assert(!this->shaderName.IsEmpty());

    if (!this->refShader.isvalid())
    {
        const nRenderPath2* renderPath = nSceneServer::Instance()->GetRenderPath();
        n_assert(renderPath);
        int shaderIndex = renderPath->FindShaderIndex(this->shaderName);
        n_assert2(-1 != shaderIndex, nString("Shader \"" + this->shaderName + "\" not found in  \"" + renderPath->GetFilename() + "\"").Get());
        const nRpShader& rpShader = renderPath->GetShader(shaderIndex);
        this->shaderIndex = rpShader.GetBucketIndex();
        this->refShader = rpShader.GetShader();
        this->refShader->AddRef();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool
nMaterialNode::LoadResources()
{
    if (this->LoadShader())
    {
        if (nAbstractShaderNode::LoadResources())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Unload the resources if refcount has reached zero.
*/
void
nMaterialNode::UnloadResources()
{
    nAbstractShaderNode::UnloadResources();
    this->UnloadShader();
}

//------------------------------------------------------------------------------
/**
    Indicate to scene server that we provide a shader.
*/
bool
nMaterialNode::HasShader() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    Setup shader attributes before rendering instances of this scene node.
*/
bool
nMaterialNode::ApplyShader(nSceneServer* sceneServer)
{
    n_assert(sceneServer);

    if (this->refShader.isvalid())
    {
    /*
        // set texture transforms
        n_assert(nGfxServer2::MaxTextureStages >= 4);
        static matrix44 m;
        this->textureTransform[0].getmatrix44(m);
        gfxServer->SetTransform(nGfxServer2::Texture0, m);
        this->textureTransform[1].getmatrix44(m);
        gfxServer->SetTransform(nGfxServer2::Texture1, m);

        // transfer shader parameters en block
        // FIXME: this should be split into instance-variables
        // and instance-set-variables
        shader->SetParams(this->shaderParams);
    */

        // if there are no animators on this node, we only
        // need to set shader params once for the whole instance batch
        if (this->GetNumAnimators() == 0)
        {
            this->refShader->SetParams(this->shaderParams);
        }
        nGfxServer2::Instance()->SetShader(this->refShader);
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
    nShader2* shader = this->refShader;
    nGfxServer2* gfxServer = nGfxServer2::Instance();

    // FIXME FIXME FIXME
    // THIS IS A LARGE PERFORMANCE BOTTLENECK!
    // NEED TO SPLIT SHADER PARAMETERS INTO STATIC "INSTANCE SET" PARAMETERS
    // AND LIGHTWEIGHT "PER-INSTANCE" PARAMETERS WHICH CAN BE ANIMATED!!!
    n_assert(sceneServer);
    n_assert(renderContext);

    // invoke shader manipulators
    if (this->GetNumAnimators() > 0)
    {
        this->InvokeAnimators(nAnimator::Shader, renderContext);
        shader->SetParams(this->shaderParams);
    }

    // set texture transforms
//    n_assert(nGfxServer2::MaxTextureStages >= 4);
//    static matrix44 m;
//    this->textureTransform[0].getmatrix44(m);
//    gfxServer->SetTransform(nGfxServer2::Texture0, m);
//    this->textureTransform[1].getmatrix44(m);
//    gfxServer->SetTransform(nGfxServer2::Texture1, m);

    // set shader override parameters from render context (set directly by application)
    shader->SetParams(renderContext->GetShaderOverrides());

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
nMaterialNode::IsTextureUsed(nShaderState::Param param)
{
    if (this->refShader.isvalid())
    {
        if (this->refShader->IsParameterUsed(param))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    This updates the provided instance stream object by appending instance
    variables of all our shaders to the stream declaration.
*/
void
nMaterialNode::UpdateInstStreamDecl(nInstanceStream::Declaration& decl)
{
    // load shaders if not happened yet
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    this->refShader->UpdateInstanceStreamDecl(decl);
}
