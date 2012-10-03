#ifndef N_TRANSFORMCURVEANIMATOR_H
#define N_TRANSFORMCURVEANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nTransformCurveAnimator
    @ingroup Scene

    @brief A transform curve animator controls a scene node's animation from
    an animation curve of the Animation Subsystem.

    See also @ref N2ScriptInterface_ntransformcurveanimator

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nanimator.h"
#include "anim2/nanimation.h"
#include "anim2/nanimationserver.h"

//------------------------------------------------------------------------------
class nTransformCurveAnimator : public nAnimator
{
public:
    /// constructor
    nTransformCurveAnimator();
    /// destructor
    virtual ~nTransformCurveAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// set the animation resource name
    void SetAnimation(const char* name);
    /// get the animation resource name
    const char* GetAnimation() const;
    /// set the animation group to use
    void SetAnimationGroup(int group);
    /// get the animation group to use
    int GetAnimationGroup();
    /// return the type of this animator object (TRANSFORM)
    virtual Type GetAnimatorType() const;
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
*/
inline
void
nTransformCurveAnimator::SetAnimationGroup(int group)
{
    this->animationGroup = group;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTransformCurveAnimator::GetAnimationGroup()
{
    return this->animationGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTransformCurveAnimator::GetAnimation() const
{
    return this->animationName.Get();
}

//------------------------------------------------------------------------------
#endif

