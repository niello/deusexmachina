#ifndef N_BLENDSHAPEANIMATOR_H
#define N_BLENDSHAPEANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nBlendShapeAnimator
    @ingroup Scene

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nanimator.h"
#include "anim2/nanimation.h"
#include "anim2/nanimationserver.h"

//------------------------------------------------------------------------------
class nBlendShapeAnimator : public nAnimator
{
public:
    /// constructor
    nBlendShapeAnimator();
    /// destructor
    virtual ~nBlendShapeAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();
    /// return the type of this animator object
    virtual Type GetAnimatorType() const;

    /// set the animation resource name
    void SetAnimation(const nString& name);
    /// get the animation resource name
    const nString& GetAnimation();
    /// set the animation group to use
    void SetAnimationGroup(int group);
    /// get which animation group is used
    int GetAnimationGroup();
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);

private:
    /// load animation resource
    bool LoadAnimation();
    /// unload animation resource
    void UnloadAnimation();

    int animationGroup;
    nString animationName;

    nRef<nAnimation> refAnimation;
};

//------------------------------------------------------------------------------
/**
    Set the resource name. The animation resource name consists of the
    filename of the animation.
*/
inline
void
nBlendShapeAnimator::SetAnimation(const nString& name)
{
    n_assert(name.IsValid());
    this->UnloadAnimation();
    this->animationName = name;
}

//------------------------------------------------------------------------------
/**
    Get the animation resource name.
*/
inline
const nString&
nBlendShapeAnimator::GetAnimation()
{
    return this->animationName;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nBlendShapeAnimator::SetAnimationGroup(int group)
{
    this->animationGroup = group;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nBlendShapeAnimator::GetAnimationGroup()
{
    return this->animationGroup;
}

//------------------------------------------------------------------------------
#endif

