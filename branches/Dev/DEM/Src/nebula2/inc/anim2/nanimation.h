#ifndef N_ANIMATION_H
#define N_ANIMATION_H
//------------------------------------------------------------------------------
/**
    @class nAnimation
    @ingroup Anim2

    @brief An nAnimation is a nResource subclass which holds one or more
    animation curves, which must all share the same number of
    keys, frame rate and loop type. The nAnimation object
    can be sampled for a given curve index at a given sample time.

    One nAnimation object points to the data of exactly one
    "animation group" in an animation file (an animation group is
    a collection of curves that share the same attributes (number of keys,
    frame rate and looping type). Several nAnimation objects may
    access one animation file (since one animation file may contain
    several animation groups).

    Subclasses of nAnimation may provide file streaming or
    sampling in-memory animation data.

    After creation the object should be configured by setting the
    file and group name, after that, LoadResource() should
    be called (and also when IsValid() returns false), this is
    normal nResource class behavior.

    To get a sample from a curve, call the SampleCurves() method.

    Animation file formats:

    <b>ASCII: .nanim2      (not streamable)</b>

    @verbatim
    type nanim2
    numgroups [numGroups]
    numkeys [numKeys]

    FOR EACH GROUP {
        group [numCurves] [startKey] [numKeys] [keyStride] [keyTime] [FadeInFrames] [clamp|repeat]
        FOR EACH CURVE {
            curve [step|linear|quat|none] [firstKeyIndex] [collapsedKey(x,y,z,w)]
            curve
            ...
        }
        FOR EACH KEY (Keys are interleaved) {
            key [x y z w]   // first key of first non-collapsed curve
            key [x y z w]   // first key of second non-collapsed curve
            key [x y z w]   // first key of third non-collapsed curve
            ...             // etc...
        }
    }
    @endverbatim

    <b>BINARY: .nax2   (optionally streamable)</b>

    @verbatim
    HEADER {
        int32 magic         // NAX2
        int32 numGroups     // number of groups in file
        int32 numKeys       // number of keys in file
    }

    FOR EACH GROUP {
        int32 numCurves         // number of curves in group
        int32 startKey          // first key index
        int32 numKeys           // number of keys in group
        int32 keyStride         // key stride in key pool
        float keyTime           // key duration
        float fadeInFrames      // number of fade in frames
        int32 loopType          // nAnimation::LoopType
    }

    FOR EACH CURVE {
        int32 ipolType          // nAnimation::Curve::IpolType
        int32 firstKeyIndex     // index of first curve key in key pool (-1 if collapsed!)
        int32 isAnimated        // flag, if the curve's joint is animated
        float[4] collapsedKey   // the key value if this is a collapsed curve
    }

    FOR EACH KEY {
        float[4] value
    }
    @endverbatim

    (C) 2003 RadonLabs GmbH
*/
#include "resource/nresource.h"
#include "util/narray.h"

//------------------------------------------------------------------------------
class nAnimation : public nResource
{
public:
    /// holds animation curve information
    class Curve
    {
    public:
        /// interpolation type
        enum IpolType
        {
            None = 0,       ///< always return the const key
            Step,           ///< no interpolation done between keys
            Quat,           ///< do quaternion interpolation
            Linear,         ///< linear interpolation done between keys
        };

        /// constructor
        Curve();
        /// set the interpolation type
        void SetIpolType(IpolType t);
        /// get the interpolation type
        IpolType GetIpolType() const;
        /// set the curve's const value
        void SetConstValue(const vector4& val);
        /// get the curve's const value
        const vector4& GetConstValue() const;
        /// convert a string to an ipol type
        static IpolType StringToIpolType(const char* str);
        /// set index of the first key in the global key array
        void SetFirstKeyIndex(int index);
        /// get first key index
        int GetFirstKeyIndex() const;
        /// set curve as animated or not
        void SetIsAnimated(int isAnim);
        /// is curve animated?
        int IsAnimated() const;
        /// set the start value (at frame 0/ time 0) of the curve
        void SetStartValue(vector4 val);
        /// get the start value of the curve
        vector4 GetStartValue() const;
        /// set the animation value of the curve if it is animated
        void SetCurAnimClipValue(vector4 value);
        /// get the animation value of the curve
        vector4 GetCurAnimClipValue() const;

    private:
        IpolType ipolType;
        vector4 constValue;
        int firstKeyIndex;
        int isAnimated;
        vector4 startValue;
        vector4 firstClipValue;
    };

    /// holds animation group information (a group of curves)
    class Group
    {
    public:
        /// loop types
        enum LoopType
        {
            Clamp = 0,
            Repeat,
            InvalidLoopType,
        };

        /// constructor
        Group();
        /// set number of curves in group
        void SetNumCurves(int c);
        /// get number of curves in group
        int GetNumCurves() const;
        /// a curve object at index
        Curve& GetCurveAt(int i) const;
        /// set start key
        void SetStartKey(int k);
        /// get start key
        int GetStartKey() const;
        /// set number of keys (identical for all curves)
        void SetNumKeys(int k);
        /// get number of keys
        int GetNumKeys() const;
        /// set the key stride
        void SetKeyStride(int stride);
        /// get the key stride
        int GetKeyStride() const;
        /// set the key duration
        void SetKeyTime(float t);
        /// get the key duration
        float GetKeyTime() const;
        /// set the number of fade in frames
        void SetFadeInFrames(float frames);
        ///get the number of fade in frames
        float GetFadeInFrames() const;
        /// set the loop type (identical for all curves)
        void SetLoopType(LoopType t);
        /// get the loop type
        LoopType GetLoopType() const;
        /// convert string to loop type
        static LoopType StringToLoopType(const char* str);
        /// convert a time stamp into 2 global key indexes and an inbetween value
        void TimeToIndex(float time, int& keyIndex0, int& keyIndex1, float& inbetween) const;
        /// return true if time is between startTime and stopTime (handles looped and clamped case correctly)
        bool IsInbetween(float time, float startTime, float stopTime) const;
        /// get animation duration
        nTime GetDuration() const;

    private:
        int startKey;           ///< index of start key
        int numKeys;            ///< number of keys shared by all curves
        int keyStride;          ///< key stride in key array
        float keyTime;          ///< number of keys
        LoopType loopType;      ///< the loop type
        float fadeInFrames;
        nArray<Curve> curveArray;
    };

    /// constructor
	nAnimation(): groupArray(0, 0) {}
    /// destructor
	virtual ~nAnimation() { if (!IsUnloaded()) Unload(); }
    /// sample values from curve range
	virtual void SampleCurves(float time, int groupIndex, int firstCurveIndex, int numCurves, vector4* keyArray) {}
    /// get duration of entire animation
	nTime GetDuration(int groupIndex) const { return groupArray[groupIndex].GetDuration(); }
    /// set number of groups in animation
    void SetNumGroups(int g);
    /// get number of groups in animation
    int GetNumGroups() const;
    /// get group at index
    Group& GetGroupAt(int i) const;

protected:

	virtual void UnloadResource() { groupArray.Clear(); }
    void FixKeyOffsets();

    nArray<Group> groupArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Curve::Curve() :
    ipolType(None),
    firstKeyIndex(-1),
    isAnimated(1),
    startValue(0.0f, 0.0f, 0.0f, 0.0f),
    firstClipValue(0.0f, 0.0f, 0.0f, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetIpolType(IpolType t)
{
    // n_assert(t != None);
    this->ipolType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Curve::IpolType
nAnimation::Curve::GetIpolType() const
{
    return this->ipolType;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetConstValue(const vector4& val)
{
    this->constValue = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nAnimation::Curve::GetConstValue() const
{
    return this->constValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetFirstKeyIndex(int index)
{
    this->firstKeyIndex = index;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Curve::GetFirstKeyIndex() const
{
    return this->firstKeyIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Curve::IpolType
nAnimation::Curve::StringToIpolType(const char* str)
{
    n_assert(str);
    if (0 == strcmp("none", str))       return None;
    if (0 == strcmp("step", str))       return Step;
    if (0 == strcmp("quat", str))       return Quat;
    return Linear;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetIsAnimated(int isAnim)
{
    this->isAnimated = isAnim;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Curve::IsAnimated() const
{
    return this->isAnimated;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetStartValue(vector4 value)
{
    this->startValue = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4
nAnimation::Curve::GetStartValue() const
{
    return this->startValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Curve::SetCurAnimClipValue(vector4 value)
{
    this->firstClipValue = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4
nAnimation::Curve::GetCurAnimClipValue() const
{
    return this->firstClipValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Group::Group() :
    startKey(0),
    numKeys(0),
    keyStride(0),
    keyTime(0),
    loopType(Repeat),
    fadeInFrames(0),
    curveArray(0, 0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetNumCurves(int c)
{
    this->curveArray.SetFixedSize(c);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Group::GetNumCurves() const
{
    return this->curveArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Curve&
nAnimation::Group::GetCurveAt(int index) const
{
    return this->curveArray[index];
}


//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetStartKey(int k)
{
    this->startKey = k;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Group::GetStartKey() const
{
    return this->startKey;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetNumKeys(int k)
{
    this->numKeys = k;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Group::GetNumKeys() const
{
    return this->numKeys;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetKeyStride(int stride)
{
    this->keyStride = stride;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::Group::GetKeyStride() const
{
    return this->keyStride;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetKeyTime(float t)
{
    this->keyTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimation::Group::GetKeyTime() const
{
    return this->keyTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetFadeInFrames(float frames)
{
    this->fadeInFrames = frames;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimation::Group::GetFadeInFrames() const
{
    return this->fadeInFrames;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::Group::SetLoopType(LoopType t)
{
    this->loopType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Group::LoopType
nAnimation::Group::GetLoopType() const
{
    return this->loopType;
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Group::LoopType
nAnimation::Group::StringToLoopType(const char* str)
{
    n_assert(str);
    if (0 == strcmp("repeat", str))     return Repeat;
    return Clamp;
}

//------------------------------------------------------------------------------
/**
    Return true if absolute time is between relative start and stop time.
    This will handle the case where the curve is looped and time is out
    of bounds correctly.

    Convert an absolute time into a normalized time. If the group is looped,
    this will return a time within start and end.
*/
inline
bool
nAnimation::Group::IsInbetween(float time, float startTime, float stopTime) const
{
    if (Clamp == this->loopType)
    {
        return ((time >= startTime) && (time <= stopTime));
    }
    float dur = float(this->GetDuration());
    float normStartTime = fmodf(startTime, dur);
    float normStopTime  = fmodf(stopTime, dur);
    if (normStartTime < normStopTime)
    {
        return time >= normStartTime && time <= normStopTime;
    }
    // wrap around
    return time >= normStartTime || time <= normStopTime;
}

//------------------------------------------------------------------------------
/**
    - 18-Oct-04   floh      fixed case where returned indexes could be <0
*/
inline
void
nAnimation::Group::TimeToIndex(float time, int& keyIndex0, int& keyIndex1, float& inbetween) const
{
    float frame  = time / this->keyTime;
    int intFrame = int(frame);
    keyIndex0    = intFrame - this->startKey;
    keyIndex1    = keyIndex0 + 1;
    inbetween    = n_saturate(frame - float(intFrame));
    if (Clamp == this->loopType)
    {
        // 'clamp' loop type
        if (keyIndex0 < 0)                   keyIndex0 = 0;
        else if (keyIndex0 >= this->numKeys) keyIndex0 = this->numKeys - 1;
        if (keyIndex1 < 0)                   keyIndex1 = 0;
        else if (keyIndex1 >= this->numKeys) keyIndex1 = this->numKeys - 1;
    }
    else
    {
        // 'repeat' loop type
        keyIndex0 %= this->numKeys;
        keyIndex1 %= this->numKeys;
        if (keyIndex0 < 0)
        {
            keyIndex0 += this->numKeys;
        }
        if (keyIndex1 < 0)
        {
            keyIndex1 += this->numKeys;
        }
    }
    n_assert((keyIndex0 >= 0) && (keyIndex0 < this->numKeys));
    n_assert((keyIndex1 >= 0) && (keyIndex1 < this->numKeys));

    keyIndex0 *= this->keyStride;
    keyIndex1 *= this->keyStride;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTime
nAnimation::Group::GetDuration() const
{
    return this->numKeys * this->keyTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimation::SetNumGroups(int numGroups)
{
    this->groupArray.SetFixedSize(numGroups);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimation::GetNumGroups() const
{
    return this->groupArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimation::Group&
nAnimation::GetGroupAt(int i) const
{
    return this->groupArray[i];
}

//------------------------------------------------------------------------------
#endif
