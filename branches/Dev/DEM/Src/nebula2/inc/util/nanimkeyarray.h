#ifndef N_ANIMKEYARRAY_H
#define N_ANIMKEYARRAY_H
//------------------------------------------------------------------------------
/**
    @class nAnimKeyArray
    @ingroup Util
    @brief A specialized nArray for working with sparsely set animation keys
    (no fixed frame rate). Note: TYPE must be a version of nAnimKey!

    (C) 2005 Radon Labs GmbH
*/
#include "util/narray.h"
#include "util/nanimkey.h"
#include "util/nanimlooptype.h"

//------------------------------------------------------------------------------
template<class TYPE> class nAnimKeyArray : public nArray<TYPE>
{
public:
    /// constructor with default parameters
    nAnimKeyArray();
    /// constuctor with initial size and grow size
    nAnimKeyArray(int initialSize, int initialGrow);
    /// get sampled key
    bool Sample(float sampleTime, nAnimLoopType::Type loopType, TYPE& result);
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nAnimKeyArray<TYPE>::nAnimKeyArray() :
    nArray<TYPE>()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nAnimKeyArray<TYPE>::nAnimKeyArray(int initialSize, int initialGrow) :
    nArray<TYPE>(initialSize, initialGrow)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
nAnimKeyArray<TYPE>::Sample(float sampleTime, nAnimLoopType::Type loopType, TYPE& result)
{
    if (this->Size() > 1)
    {
        float minTime = this->Front().GetTime();
        float maxTime = this->Back().GetTime();
        if (maxTime > 0.0f)
        {
            if (nAnimLoopType::Loop == loopType)
            {
                // in loop mode, wrap time into loop time
                sampleTime = sampleTime - (float(floor(sampleTime / maxTime)) * maxTime);
            }

            // clamp time to range
            if (sampleTime < minTime)       sampleTime = minTime;
            else if (sampleTime >= maxTime) sampleTime = maxTime - 0.001f;

            // find the surrounding keys
            n_assert(this->Front().GetTime() == 0.0f);
            int i = 0;
            while ((*this)[i].GetTime() <= sampleTime)
            {
                i++;
            }
            n_assert((i > 0) && (i < this->Size()));

            const TYPE& key0 = (*this)[i - 1];
            const TYPE& key1 = (*this)[i];
            float time0 = key0.GetTime();
            float time1 = key1.GetTime();

            // compute the actual interpolated values
            float lerpTime;
            if (time1 > time0) lerpTime = (float) ((sampleTime - time0) / (time1 - time0));
            else               lerpTime = 1.0f;

            result.Lerp(key0.GetValue(), key1.GetValue(), lerpTime);
            result.SetTime(sampleTime);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
#endif
