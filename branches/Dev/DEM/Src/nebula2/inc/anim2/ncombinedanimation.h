#ifndef N_COMBINEDANIMATION_H
#define N_COMBINEDANIMATION_H
//------------------------------------------------------------------------------
/**
    @class nCombinedAnimation
    @ingroup Anim2

    @brief A combined animation is an animation which ONLY can be created from
    existing nMemoryAnimations.

    See the parent class nMemoryAnimation for more info.

    FIXME FLOH: This should be rewritten to directly scatter-load animations
    from a complete directory. This would simplify the code in
    ncharacter3skinanimator a lot!

    (C) 2005 RadonLabs GmbH
*/
#include "anim2/nmemoryanimation.h"

//------------------------------------------------------------------------------
class nCombinedAnimation : public nMemoryAnimation
{
public:
    /// constructor
    nCombinedAnimation();
    /// destructor
    virtual ~nCombinedAnimation();
    /// begin array of animations to combine
    void BeginAnims();
    /// add an animation to be combined
    void AddAnim(nMemoryAnimation* animation);
    /// end animations to combine (and perform combining)
    void EndAnims();
private:
    /// load the resource (sets the valid flag)
    virtual bool LoadResource();
    /// unload the resource (clears the valid flag)
    virtual void UnloadResource();

    nArray<nRef<nMemoryAnimation> > animPtrs;
};
//------------------------------------------------------------------------------
#endif
