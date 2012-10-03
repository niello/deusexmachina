#ifndef N_IPOLKEYARRAY_H
#define N_IPOLKEYARRAY_H
//------------------------------------------------------------------------------
/**
    @class nIpolKeyArray
    @ingroup NebulaDataTypes

    @brief An Array for interpolation between keys, used by the animators
    for a linear interpolation the data type.

    The function
    void lerp<TYPE>(TYPE & result, const TYPE & val0,
                    const TYPE & val1, float lerpVal)
    must be defined. These are already defined for vectors in vector.h
    and int and float in nmath.h.

    This file is licensed under the terms of the Nebula License.

    (C) 2004 Johannes Kellner
*/
#include "kernel/ntypes.h"
#include "scene/nanimator.h"
#include "mathlib/nmath.h"

//------------------------------------------------------------------------------
template<class TYPE>
class nIpolKeyArray
{
public:
    nIpolKeyArray()
    {
        //empty
    }

    /// constructor with initial size and grow size of the internal elements nArray
    nIpolKeyArray(int initialSize, int grow) :
        keyArray(initialSize, grow)
    {
        //empty
    }

    ~nIpolKeyArray()
    {
        //empty
    }

    ///get interpolated key value at time
    bool SampleKey(float time, TYPE& result, const nAnimLoopType::Type loopType) const;

    /// add a key
    void AddKey(float t, const TYPE& key)
    {
        Key newKey(t, key);
        this->keyArray.Append(newKey);
    }

    /// get number of keys
    int  GetNumKeys() const
    {
        return this->keyArray.Size();
    }

    /// get key at index
    void GetKeyAt(const int index, float& time, TYPE& key) const
    {
        const Key& k = this->keyArray[index];
        time = k.time;
        key  = k.value;
    }

private:
    class Key
    {
    public:
        Key() :
            time(0.0f)
        {
            //empty
        }
        Key(float t, const TYPE& v) :
            time(t), value(v)
        {
            //empty
        }

        ~Key()
        {
            //empty
        }

        float time;
        TYPE value;
    };

    nArray<Key> keyArray;
};

//------------------------------------------------------------------------------
/**
    Compute an interpolated key value from a time and a key array.

    @param  sampleTime  [in] time position at which to sample
    @param  result      [out] the result
    @return             true if the result is valid, false otherwise
*/
template<class TYPE>
inline bool
nIpolKeyArray<TYPE>::SampleKey(float sampleTime, TYPE& result, const nAnimLoopType::Type loopType) const
{
    if (keyArray.Size() > 1)
    {
        float minTime = keyArray.Front().time;
        float maxTime = keyArray.Back().time;
        if (maxTime > 0.0f)
        {
            if (loopType == nAnimLoopType::Loop)
            {
                // in loop mode, wrap time into loop time
                sampleTime = sampleTime - (float(floor(sampleTime / maxTime)) * maxTime);
            }

            // clamp time to range
            if (sampleTime < minTime)       sampleTime = minTime;
            else if (sampleTime >= maxTime) sampleTime = maxTime - 0.001f;

            // find the surrounding keys
            n_assert(keyArray.Front().time == 0.0f);
            int i = 0;;
            while (keyArray[i].time <= sampleTime)
            {
                i++;
            }
            n_assert((i > 0) && (i < keyArray.Size()));

            const Key& key0 = keyArray[i - 1];
            const Key& key1 = keyArray[i];
            float time0 = key0.time;
            float time1 = key1.time;

            // compute the actual interpolated values
            float lerpVal;
            if (time1 > time0) lerpVal = (float) ((sampleTime - time0) / (time1 - time0));
            else               lerpVal = 1.0f;

            lerp<TYPE>(result, key0.value, key1.value, lerpVal);
            return true;
        }
    }
    return false;
}

#endif

