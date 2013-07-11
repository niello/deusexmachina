#ifndef N_VECTOR_H
#define N_VECTOR_H

#include <mathlib/vector2.h>
#include <mathlib/vector3.h>
#include <mathlib/vector4.h>

template<> static inline void lerp<vector2>(vector2 & result, const vector2 & val0, const vector2 & val1, float lerpVal)
{
	result.lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

template<> static inline void lerp<vector3>(vector3 & result, const vector3 & val0, const vector3 & val1, float lerpVal)
{
	result.lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

template<> static inline void lerp<vector4>(vector4 & result, const vector4 & val0, const vector4 & val1, float lerpVal)
{
	result.lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

#endif
