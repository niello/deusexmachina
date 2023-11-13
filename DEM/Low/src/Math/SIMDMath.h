#pragma once
#include <Math/Vector4.h>
#include <Math/Quaternion.h>
#include <rtm/vector4f.h>
#include <rtm/quatf.h>

// Math for view and projection calculations

namespace Math
{
constexpr U8 ClipInside = (1 << 0);
constexpr U8 ClipOutside = (1 << 1);
constexpr U8 ClipIntersect = ClipInside | ClipOutside;

DEM_FORCE_INLINE rtm::vector4f ToSIMD(const vector3& v) noexcept
{
	return rtm::vector_set(v.x, v.y, v.z);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::vector4f ToSIMD(const vector4& v) noexcept
{
	return rtm::vector_set(v.x, v.y, v.z, v.w);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::quatf ToSIMD(const quaternion& q) noexcept
{
	return rtm::quat_set(q.x, q.y, q.z, q.w);
}
//---------------------------------------------------------------------

#define RTM_MIX_ALIAS_ONE(c1, c2, c3, c4) \
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL vector_mix_##c1##c2##c3##c4(rtm::vector4f_arg0 input) { return rtm::vector_mix<rtm::mix4::c1, rtm::mix4::c2, rtm::mix4::c3, rtm::mix4::c4>(input, input); }
#define RTM_MIX_ALIAS_TWO(c1, c2, c3, c4) \
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL vector_mix_##c1##c2##c3##c4(rtm::vector4f_arg0 input0, rtm::vector4f_arg1 input1) { return rtm::vector_mix<rtm::mix4::c1, rtm::mix4::c2, rtm::mix4::c3, rtm::mix4::c4>(input0, input1); }

//RTM_MIX_ALIAS_ONE(x, x, x, x); - use rtm::vector_dup_x
//RTM_MIX_ALIAS_ONE(y, y, y, y); - use rtm::vector_dup_y
//RTM_MIX_ALIAS_ONE(z, z, z, z); - use rtm::vector_dup_z
//RTM_MIX_ALIAS_ONE(w, w, w, w); - use rtm::vector_dup_w
RTM_MIX_ALIAS_ONE(x, y, x, y);
RTM_MIX_ALIAS_TWO(x, y, z, a);
RTM_MIX_ALIAS_TWO(x, y, z, b);
RTM_MIX_ALIAS_TWO(x, y, z, c);
RTM_MIX_ALIAS_TWO(x, y, z, d);
RTM_MIX_ALIAS_TWO(x, z, a, c);
//---------------------------------------------------------------------

}
