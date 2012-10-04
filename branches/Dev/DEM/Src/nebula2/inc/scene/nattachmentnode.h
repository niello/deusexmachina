#ifndef N_ATTACHMENTNODE_H
#define N_ATTACHMENTNODE_H
//------------------------------------------------------------------------------
/**
    @class nAttachmentNode
    @ingroup Scene

    @brief The purpose of nAttachmentNode is to provide an external point of access
    to a skeleton's joint transformation information.  The user specifies either a
    joint name or joint index, and the nAttachmentNode grabs the specified joint data,
    its transformation mirroring the joint's.

    The transformations performed on the nAttachmentNode itself are relative to the
    target bone's origin.  Calling GetTransform on nAttachmentNode will get you the
    world space matrix of the bone origin offset by the specified transformations.

    Please note that nAttachmentNode assumes its parent is the target nSkinShapeNode.

    -28-Mar-07  kims  Changed to be enable to export a max object which is under 
                      a bone. Thank Cho Jun Heung for the patch.

*/
#include "scene/nscenenode.h"
#include "scene/ntransformnode.h"
#include "scene/nskinanimator.h"
#include "scene/nskinshapenode.h"
#include "character/ncharskeleton.h"
#include "character/ncharacter2.h"
#include "scene/nrendercontext.h"
#include "mathlib/transform44.h"

//------------------------------------------------------------------------------
class nAttachmentNode : public nTransformNode
{
public:
    /// constructor
    nAttachmentNode();
    /// destructor
    virtual ~nAttachmentNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);

    /// update transform and render into scene server
    virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix);

    /// updates the final transformation, combing local matrix with joint matrix data if necessary
    virtual void UpdateJointTransform(nRenderContext* renderContext);
    /// set the skin animator
    void SetSkinAnimator(const char* path);
    /// get the skin animator
    const char* GetSkinAnimator() const;
    /// specifies the target joint by name
    void SetJointByName(const char* jointName);
    /// specifies the target joint by index
    void SetJointByIndex(unsigned int newIndex);
    ///
    int GetJointByIndex() const;

protected:
    int jointIndex;
    nDynAutoRef<nSkinAnimator> refSkinAnimator;
};

//------------------------------------------------------------------------------
#endif
