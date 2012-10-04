//------------------------------------------------------------------------------
//  nattachmentnode_main.cc
//  (C) 2004 Megan Fox
//------------------------------------------------------------------------------
#include "scene/nattachmentnode.h"
#include "scene/nsceneserver.h"
#include "scene/nskinanimator.h"
#include "gfx2/ngfxserver2.h"


nNebulaClass(nAttachmentNode, "ntransformnode");

//------------------------------------------------------------------------------
/**
*/
nAttachmentNode::nAttachmentNode() :
    jointIndex(-1)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nAttachmentNode::~nAttachmentNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Compute the resulting modelview matrix and set it in the scene
    server as current modelview matrix.
*/
bool
nAttachmentNode::RenderTransform(nSceneServer* sceneServer,
                                nRenderContext* renderContext,
                                const matrix44& parentMatrix)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    this->InvokeAnimators(nAnimator::Transform, renderContext);
    this->UpdateJointTransform(renderContext);
    if (this->GetLockViewer())
    {
        // if lock viewer active, copy viewer position
        const matrix44& viewMatrix = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);
        matrix44 m = this->GetTransform();
        m.M41 = viewMatrix.M41;
        m.M42 = viewMatrix.M42;
        m.M43 = viewMatrix.M43;
        sceneServer->SetModelTransform(m);
    }
    else
    {
        // default case
        sceneServer->SetModelTransform(this->GetTransform());
    }

    return true;

}

//------------------------------------------------------------------------------
/**
    Compute the final transformation matrix for the nAttachmentNode
*/
void
nAttachmentNode::UpdateJointTransform(nRenderContext* renderContext)
{
    if (this->jointIndex != -1 )
    {
        nKernelServer::Instance()->PushCwd(this);
        if( this->refSkinAnimator.isvalid() )
        {
            this->refSkinAnimator->Animate(this, renderContext);
            nVariable& charVar = renderContext->GetLocalVar( this->refSkinAnimator->GetCharacterVarIndexHandle() );
            nCharacter2* char2 = (nCharacter2*)charVar.GetObj();
            this->tform.setmatrix( char2->GetSkeleton().GetJointAt(this->jointIndex).GetMatrix() );
        }
        nKernelServer::Instance()->PopCwd();
    }
}

//------------------------------------------------------------------------------
/**
    Set relative path to the skin animator object.
*/
void
nAttachmentNode::SetSkinAnimator(const char* path)
{
    n_assert(path);
    this->refSkinAnimator = path;
}

//------------------------------------------------------------------------------
/**
    Get relative path to the skin animator object
*/
const char*
nAttachmentNode::GetSkinAnimator() const
{
    return this->refSkinAnimator.getname();
}


//------------------------------------------------------------------------------
/**
    Specifies the target joint by name

    @param jointName  the name of the joint index to target
*/
void
nAttachmentNode::SetJointByName(const char* jointName)
{
    nKernelServer::Instance()->PushCwd(this);
    if( this->refSkinAnimator.isvalid() )
    {
        this->jointIndex = this->refSkinAnimator->GetJointByName(jointName);
        if (this->jointIndex == -1)
        {
            matrix44 matIdent;
            this->tform.setmatrix(matIdent);            
        }
    }
    else
    {
        n_printf("Error: invalid skinanimator\n");
    }
    nKernelServer::Instance()->PopCwd();
    
    
}

//------------------------------------------------------------------------------
/**
    Specifies the target joint by joint index

    @param newIndex  the joint index to target
*/
void
nAttachmentNode::SetJointByIndex(unsigned int newIndex)
{
    this->jointIndex = newIndex;
    if( newIndex == -1 )
    {
        matrix44 matIdent;
        this->tform.setmatrix(matIdent);
    }
}

//------------------------------------------------------------------------------
/**
*/
int
nAttachmentNode::GetJointByIndex() const
{
    return this->jointIndex;
}
