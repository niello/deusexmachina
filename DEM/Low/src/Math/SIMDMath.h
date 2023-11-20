#pragma once
#include <Math/Matrix44.h>
#include <rtm/matrix3x4f.h>

// Math for view and projection calculations

namespace Math
{

DEM_FORCE_INLINE rtm::vector4f ToSIMD(const vector3& v) noexcept
{
	return rtm::vector_load3(v.v);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::vector4f ToSIMD(const vector4& v) noexcept
{
	return rtm::vector_load(v.v);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::quatf ToSIMD(const quaternion& q) noexcept
{
	return rtm::quat_load(q.v);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::matrix3x4f ToSIMD(const matrix44& m) noexcept
{
	return rtm::matrix3x4f
	{
		ToSIMD(m.AxisX()),
		ToSIMD(m.AxisY()),
		ToSIMD(m.AxisZ()),
		ToSIMD(m.Translation())
	};
}
//---------------------------------------------------------------------

// TODO: try overloading by return value like in RTM?
DEM_FORCE_INLINE vector3 FromSIMD3(rtm::vector4f_arg0 v) noexcept
{
	return vector3(rtm::vector_get_x(v), rtm::vector_get_y(v), rtm::vector_get_z(v));
}
//---------------------------------------------------------------------

// TODO: try overloading by return value like in RTM?
DEM_FORCE_INLINE vector4 FromSIMD4(rtm::vector4f_arg0 v) noexcept
{
	return vector4(rtm::vector_get_x(v), rtm::vector_get_y(v), rtm::vector_get_z(v), rtm::vector_get_w(v));
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE quaternion FromSIMD(rtm::quatf_arg0 v) noexcept
{
	return quaternion(rtm::quat_get_x(v), rtm::quat_get_y(v), rtm::quat_get_z(v), rtm::quat_get_w(v));
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE matrix44 FromSIMD(rtm::matrix3x4f_arg0 m) noexcept
{
	matrix44 result
	(
		FromSIMD4(m.x_axis),
		FromSIMD4(m.y_axis),
		FromSIMD4(m.z_axis),
		FromSIMD4(m.w_axis)
	);
	result.m[0][3] = 0.f;
	result.m[1][3] = 0.f;
	result.m[2][3] = 0.f;
	result.m[3][3] = 1.f;
	return result;
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
