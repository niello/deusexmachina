#pragma once
#include <Math/Matrix44.h>
#include <Math/TransformSRT.h>
#include <rtm/matrix3x4f.h>
#include <rtm/qvvf.h>

// Math for view and projection calculations

namespace Math
{

struct CLine
{
	rtm::vector4f Start;
	rtm::vector4f Dir;
};

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f ToSIMD(const vector3& v) noexcept
{
	return rtm::vector_load3(v.v);
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f ToSIMD(const vector4& v) noexcept
{
	return rtm::vector_load(v.v);
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::quatf ToSIMD(const quaternion& q) noexcept
{
	return rtm::quat_load(q.v);
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::qvvf ToSIMD(const CTransformSRT& Tfm) noexcept
{
	return rtm::qvv_set(ToSIMD(Tfm.Rotation), ToSIMD(Tfm.Translation), ToSIMD(Tfm.Scale));
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::matrix3x4f ToSIMD(const matrix44& m) noexcept
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
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector3 RTM_SIMD_CALL FromSIMD3(rtm::vector4f_arg0 v) noexcept
{
	return vector3(rtm::vector_get_x(v), rtm::vector_get_y(v), rtm::vector_get_z(v)); // rtm::vector_store3 does the same thing
}
//---------------------------------------------------------------------

// TODO: try overloading by return value like in RTM?
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4 RTM_SIMD_CALL FromSIMD4(rtm::vector4f_arg0 v) noexcept
{
	vector4 Result;
	rtm::vector_store(v, Result.v);
	return Result;
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quaternion RTM_SIMD_CALL FromSIMD(rtm::quatf_arg0 q) noexcept
{
	quaternion Result;
	rtm::quat_store(q, Result.v);
	return Result;
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE CTransformSRT RTM_SIMD_CALL FromSIMD(rtm::qvvf_arg0 Tfm) noexcept
{
	return CTransformSRT{ FromSIMD3(Tfm.scale), FromSIMD(Tfm.rotation), FromSIMD3(Tfm.translation) };
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE matrix44 RTM_SIMD_CALL FromSIMD(rtm::matrix3x4f_arg0 m) noexcept
{
	matrix44 Result
	(
		FromSIMD4(m.x_axis),
		FromSIMD4(m.y_axis),
		FromSIMD4(m.z_axis),
		FromSIMD4(m.w_axis)
	);
	Result.m[0][3] = 0.f;
	Result.m[1][3] = 0.f;
	Result.m[2][3] = 0.f;
	Result.m[3][3] = 1.f;
	return Result;
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE matrix44 RTM_SIMD_CALL FromSIMD(rtm::matrix4x4f_arg0 m) noexcept
{
	return matrix44(FromSIMD4(m.x_axis), FromSIMD4(m.y_axis), FromSIMD4(m.z_axis), FromSIMD4(m.w_axis));
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL matrix_extract_scale(rtm::matrix3x4f_arg0 m) noexcept
{
	const float ScaleX = rtm::vector_length3(m.x_axis);
	const float ScaleY = rtm::vector_length3(m.y_axis);
	const float ScaleZ = rtm::vector_length3(m.z_axis);
	return rtm::vector_set(ScaleX, ScaleY, ScaleZ);
}
//---------------------------------------------------------------------

//!!!TODO: replace with rtm::qvv_from_matrix() when it's available!
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::qvvf RTM_SIMD_CALL qvv_from_matrix(rtm::matrix3x4f_arg0 m) noexcept
{
	const rtm::vector4f Scale = matrix_extract_scale(m);

	rtm::matrix3x3f RotationMatrix = rtm::matrix_cast(m);
	constexpr float THRESHOLD = 1.0E-8F;
	if (rtm::vector_get_x(Scale) >= THRESHOLD)
		RotationMatrix.x_axis = rtm::vector_div(m.x_axis, rtm::vector_dup_x(Scale));
	if (rtm::vector_get_y(Scale) >= THRESHOLD)
		RotationMatrix.y_axis = rtm::vector_div(m.y_axis, rtm::vector_dup_y(Scale));
	if (rtm::vector_get_z(Scale) >= THRESHOLD)
		RotationMatrix.z_axis = rtm::vector_div(m.z_axis, rtm::vector_dup_z(Scale));

	return rtm::qvv_set(rtm::quat_from_matrix(RotationMatrix), m.w_axis, Scale);
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
