#ifndef N_ANIMLOOPTYPE_H
#define N_ANIMLOOPTYPE_H
//------------------------------------------------------------------------------
/**
    @class nAnimLoopType
    @ingroup Util
    @brief Define animation loop types.

    (C) 2005 Radon Labs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
class nAnimLoopType
{
public:
    // loop types
    enum Type
    {
        Loop,
        Clamp,
    };

    // convert to string
    static nString ToString(nAnimLoopType::Type t);
    // convert from string
    static nAnimLoopType::Type FromString(const nString& s);
};

//------------------------------------------------------------------------------
/**
*/
inline
nString
nAnimLoopType::ToString(nAnimLoopType::Type t)
{
    switch (t)
    {
        case Loop:  return nString("loop");
        case Clamp: return nString("clamp");
        default:
            n_error("nAnimLoopType::ToString(): invalid enum value!");
            return nString("");
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimLoopType::Type
nAnimLoopType::FromString(const nString& s)
{
    if (s == "loop") return Loop;
    else if (s == "clamp") return Clamp;
    else
    {
        n_error("nAnimLoopType::ToString(): invalid loop type '%s'\n", s.Get());
        return Clamp;
    }
}

//------------------------------------------------------------------------------
#endif
