#pragma once
#if !defined(BT_USE_SSE_IN_API) && (defined (_WIN32) || defined (__i386__))
#define BT_USE_SSE_IN_API
#endif
#include <Math/SIMDMath.h>
#include <LinearMath/btTransform.h>

// Bullet physics library to DEM data convertors

// TODO: can compile with errors if RTM and Bullet use different representations. Fix then.

namespace Math
{

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::vector4f FromBullet(const btVector3& v) noexcept
{
	return rtm::vector4f{ v.get128() };
}
//---------------------------------------------------------------------

// FIXME: use overloading by return type instead of renaming?
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE btVector3 ToBullet3(rtm::vector4f_arg0 v) noexcept
{
	btVector3 Result;
	Result.set128(v); //??? rtm::vector_set_w(v, 0.f)); - is that w=0 important anywhere in Bullet?
	return Result;
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::quatf FromBullet(const btQuaternion& q) noexcept
{
	return rtm::quatf{ q.get128() };
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE btQuaternion ToBullet(rtm::quatf_arg0 q) noexcept
{
	btQuaternion Result;
	Result.set128(q);
	return Result;
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm::matrix3x4f FromBullet(const btTransform& m) noexcept
{
	return rtm::matrix_set(
		FromBullet(m.getBasis().getColumn(0)),
		FromBullet(m.getBasis().getColumn(1)),
		FromBullet(m.getBasis().getColumn(2)),
		FromBullet(m.getOrigin()));
}
//---------------------------------------------------------------------

RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE btTransform ToBullet(rtm::matrix3x4f_arg0 m) noexcept
{
	const rtm::matrix3x3f M3x3 = rtm::matrix_cast(m);
	const rtm::matrix3x3f RotationMatrix = rtm::matrix_transpose(rtm::matrix_remove_scale(M3x3));
	return btTransform(
		btMatrix3x3(
			ToBullet3(RotationMatrix.x_axis),
			ToBullet3(RotationMatrix.y_axis),
			ToBullet3(RotationMatrix.z_axis)),
		ToBullet3(m.w_axis));
}
//---------------------------------------------------------------------

}
