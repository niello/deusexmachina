#ifndef N_MEMORYANIMATION_H
#define N_MEMORYANIMATION_H
//------------------------------------------------------------------------------
/**
    @class nMemoryAnimation
    @ingroup Anim2

    @brief An nMemoryAnimation object holds its entire animation data in
    memory.  Since all the animation is in memory all the time, the animation
    can be shared between many client objects. The disadvantage is of
    course the memory footprint.

    See the parent class nAnimation for more info.

    (C) 2003 RadonLabs GmbH
*/
#include "anim2/nanimation.h"

//------------------------------------------------------------------------------
class nMemoryAnimation : public nAnimation
{
public:
    /// constructor
    nMemoryAnimation();
    /// destructor
    virtual ~nMemoryAnimation();
    /// sample value of given curve at given time
    virtual void SampleCurves(float time, int groupIndex, int firstCurveIndex, int numCurves, vector4* keyArray);
    /// get an estimated byte size of the resource data (for memory statistics)
    virtual int GetByteSize();
    /// gets the keyArray
    nArray<vector4>& GetKeyArray();

protected:
    /// load the resource (sets the valid flag)
    virtual bool LoadResource();
    /// unload the resource (clears the valid flag)
    virtual void UnloadResource();
    /// load curve group from ASCII nanim2 file
    bool LoadNanim2(const nString& filename);
    /// load curve group from binary nax2 file
    bool LoadNax2(const nString& filename);

    nArray<vector4> keyArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nArray<vector4>&
nMemoryAnimation::GetKeyArray()
{
    return this->keyArray;
}

//------------------------------------------------------------------------------
#endif
