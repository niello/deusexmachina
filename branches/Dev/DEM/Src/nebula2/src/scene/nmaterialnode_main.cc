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

nNebulaClass(nMaterialNode, "nabstractshadernode");

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
        const CFrameShader* pFrameShader = nSceneServer::Instance()->GetRenderPath();
        n_assert(pFrameShader);
        int shaderIndex = pFrameShader->FindShaderIndex(this->shaderName);
        n_assert(-1 != shaderIndex);
        const nRpShader& rpShader = pFrameShader->shaders[shaderIndex];
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
bool nMaterialNode::LoadResources()
{
	return LoadShader() && nAbstractShaderNode::LoadResources();
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
        nGfxServer2::Instance()->SetTransform(nGfxServer2::Texture0, m);
        this->textureTransform[1].getmatrix44(m);
        nGfxServer2::Instance()->SetTransform(nGfxServer2::Texture1, m);

        // transfer shader parameters en block
        // FIXME: this should be split into instance-variables
        // and instance-set-variables
        shader->SetParams(this->shaderParams);
    */

        // if there are no animators on this node, we only
        // need to set shader params once for the whole instance batch
        //if (this->GetNumAnimators() == 0)
        //{
            this->refShader->SetParams(this->shaderParams);
        //}
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

    // FIXME FIXME FIXME
    // THIS IS A LARGE PERFORMANCE BOTTLENECK!
    // NEED TO SPLIT SHADER PARAMETERS INTO STATIC "INSTANCE SET" PARAMETERS
    // AND LIGHTWEIGHT "PER-INSTANCE" PARAMETERS WHICH CAN BE ANIMATED!!!
    n_assert(sceneServer);
    n_assert(renderContext);

    // invoke shader manipulators
    //if (this->GetNumAnimators() > 0)
    //{
	// // Animate here
    //    shader->SetParams(this->shaderParams);
    //}

    // set texture transforms
//    n_assert(nGfxServer2::MaxTextureStages >= 4);
//    static matrix44 m;
//    this->textureTransform[0].getmatrix44(m);
//    nGfxServer2::Instance()->SetTransform(nGfxServer2::Texture0, m);
//    this->textureTransform[1].getmatrix44(m);
//    nGfxServer2::Instance()->SetTransform(nGfxServer2::Texture1, m);

    // set shader override parameters from render context (set directly by application)
    shader->SetParams(renderContext->GetShaderOverrides());

    return true;
}
