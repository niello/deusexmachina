#pragma once
#include <Math/Matrix44.h>
#include <Math/SIMDMath.h>
#include <System/System.h> //!!!DBG TMP! for tmp assertions, check if can be removed!
#include <rtm/matrix4x4f.h>

// Math for view and projection calculations

namespace Math
{
constexpr U8 ClipInside = (1 << 0);
constexpr U8 ClipOutside = (1 << 1);
constexpr U8 ClipIntersect = ClipInside | ClipOutside;

// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
struct CSIMDFrustum
{
	// x, y and z components of Left, Right, Bottom and Top plane normals
	rtm::vector4f LRBT_Nx;
	rtm::vector4f LRBT_Ny;
	rtm::vector4f LRBT_Nz;

	// w component of planes, which is -d, i.e. -dot(PlaneOrigin, PlaneNormal), see CalcFrustumParams for a comment
	rtm::vector4f LRBT_w;

	// Separate params for near and far planes. They are considered parallel and use the same normal vector.
	rtm::vector4f LookAxis;
	float NearPlane;
	float FarPlane;
};

DEM_FORCE_INLINE rtm::matrix4x4f RTM_SIMD_CALL matrix_ortho_rh(float w, float h, float zn, float zf)
{
	const float m22 = 1.f / (zn - zf);
	return rtm::matrix_set(
		rtm::vector_set(2.f / w, 0.f, 0.f, 0.f),
		rtm::vector_set(0.f, 2.f / h, 0.f, 0.f),
		rtm::vector_set(0.f, 0.f, m22, 0.f),
		rtm::vector_set(0.f, 0.f, zn * m22, 1.f));
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::matrix4x4f RTM_SIMD_CALL matrix_perspective_rh(float fovY, float aspect, float zn, float zf)
{
	const float h = 1.f / rtm::scalar_tan(fovY * 0.5f);
	const float m22 = zf / (zn - zf);
	return rtm::matrix_set(
		rtm::vector_set(h / aspect, 0.f, 0.f, 0.f),
		rtm::vector_set(0.f, h, 0.f, 0.f),
		rtm::vector_set(0.f, 0.f, m22, -1.f),
		rtm::vector_set(0.f, 0.f, zn * m22, 0.f));
}
//---------------------------------------------------------------------

// Extract frustum planes using Gribb-Hartmann method.
// See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
// See https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
DEM_FORCE_INLINE CSIMDFrustum RTM_SIMD_CALL CalcFrustumParams(rtm::matrix4x4f_arg0 m) noexcept
{
	// In the paper Left = W + X, Right = W - X, Bottom = W + Y, Top = W - Y. Normals look inside the frustum, i.e. positive halfspace means 'inside'.
	// We invert this to Left = -X - W, Right = X - W, Bottom = -Y - W, Top = Y - W. Normals look outside the frustum, i.e. positive halfspace means 'outside'.
	// LRBT_w is a -d, i.e. -dot(PlaneNormal, PlaneOrigin). +w instead of -d allows us using a single fma instead of separate mul & sub in clipping tests.
	constexpr rtm::vector4f SignMask{ -1.f, 1.f, -1.f, 1.f }; //???can negate sign bit by mask?
	const auto LRBT_Nx = rtm::vector_sub(rtm::vector_mul(Math::vector_mix_xyxy(m.x_axis), SignMask), rtm::vector_dup_w(m.x_axis));
	const auto LRBT_Ny = rtm::vector_sub(rtm::vector_mul(Math::vector_mix_xyxy(m.y_axis), SignMask), rtm::vector_dup_w(m.y_axis));
	const auto LRBT_Nz = rtm::vector_sub(rtm::vector_mul(Math::vector_mix_xyxy(m.z_axis), SignMask), rtm::vector_dup_w(m.z_axis));
	const auto LRBT_w =  rtm::vector_sub(rtm::vector_mul(Math::vector_mix_xyxy(m.w_axis), SignMask), rtm::vector_dup_w(m.w_axis));

	// Near and far planes are parallel, which enables us to save some work. Near & far planes are +d (-w) along the look
	// axis, so NearPlane is negated once and FarPlane is negated twice (once to make it -w, once to invert the axis).
	// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W.
	// TODO: check if it is really faster than making another vector set for NF_Nx etc. At least less SSE registers used?
	const auto NearAxis = rtm::vector_set(rtm::vector_get_z(m.x_axis), rtm::vector_get_z(m.y_axis), rtm::vector_get_z(m.z_axis), 0.f);
	const auto FarAxis = rtm::vector_sub(rtm::vector_set(rtm::vector_get_w(m.x_axis), rtm::vector_get_w(m.y_axis), rtm::vector_get_w(m.z_axis), 0.f), NearAxis);
	const float InvNearLen = rtm::scalar_cast(rtm::scalar_sqrt_reciprocal(static_cast<rtm::scalarf>(rtm::vector_length_squared3(NearAxis))));
	const auto LookAxis = rtm::vector_mul(NearAxis, InvNearLen);
	const float m32 = rtm::vector_get_z(m.w_axis);
	const float m33 = rtm::vector_get_w(m.w_axis);
	const float NearPlane = -m32 * InvNearLen;
	const float FarPlane = (m33 - m32) * rtm::scalar_cast(rtm::scalar_sqrt_reciprocal(static_cast<rtm::scalarf>(rtm::vector_length_squared3(FarAxis))));

	return { LRBT_Nx, LRBT_Ny, LRBT_Nz, LRBT_w, LookAxis, NearPlane, FarPlane };
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE bool RTM_SIMD_CALL HasIntersection(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = rtm::vector_mul_add(rtm::vector_dup_x(BoxCenter), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_y(BoxCenter), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_z(BoxCenter), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
	auto ProjectedExtent = rtm::vector_mul(rtm::vector_dup_x(BoxExtent), rtm::vector_abs(Frustum.LRBT_Nx));
	ProjectedExtent = rtm::vector_mul_add(rtm::vector_dup_y(BoxExtent), rtm::vector_abs(Frustum.LRBT_Ny), ProjectedExtent);
	ProjectedExtent = rtm::vector_mul_add(rtm::vector_dup_z(BoxExtent), rtm::vector_abs(Frustum.LRBT_Nz), ProjectedExtent);

	// Plane normals look outside the frustum
	// TODO: rtm::vector_any_greater_then, or leave vector_any_greater_equal?
	if (rtm::vector_any_greater_equal(CenterDistance, ProjectedExtent))
		return false;

	// If inside LRTB, check intersection with NF planes
	const float CenterAlongLookAxis = rtm::vector_dot3(Frustum.LookAxis, BoxCenter);
	const float ExtentAlongLookAxis = rtm::vector_dot3(rtm::vector_abs(Frustum.LookAxis), BoxExtent);
	const float ClosestPoint = CenterAlongLookAxis - ExtentAlongLookAxis;
	const float FarthestPoint = CenterAlongLookAxis + ExtentAlongLookAxis;
	return (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// Returns a 2 bit mask with bit0 / bit1 set if the AABB is present inside / outside the frustum.
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE U8 RTM_SIMD_CALL ClipAABB(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = rtm::vector_mul_add(rtm::vector_dup_x(BoxCenter), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_y(BoxCenter), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_z(BoxCenter), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
	auto ProjectedExtent = rtm::vector_mul(rtm::vector_dup_x(BoxExtent), rtm::vector_abs(Frustum.LRBT_Nx));
	ProjectedExtent = rtm::vector_mul_add(rtm::vector_dup_y(BoxExtent), rtm::vector_abs(Frustum.LRBT_Ny), ProjectedExtent);
	ProjectedExtent = rtm::vector_mul_add(rtm::vector_dup_z(BoxExtent), rtm::vector_abs(Frustum.LRBT_Nz), ProjectedExtent);

	// Plane normals look outside the frustum
	bool HasVisiblePart = rtm::vector_all_less_equal(CenterDistance, ProjectedExtent);
	bool HasInvisiblePart = false;
	if (HasVisiblePart)
	{
		// If inside LRTB, check intersection with NF planes
		const float CenterAlongLookAxis = rtm::vector_dot3(Frustum.LookAxis, BoxCenter);
		const float ExtentAlongLookAxis = rtm::vector_dot3(rtm::vector_abs(Frustum.LookAxis), BoxExtent);
		const float ClosestPoint = CenterAlongLookAxis - ExtentAlongLookAxis;
		const float FarthestPoint = CenterAlongLookAxis + ExtentAlongLookAxis;
		HasVisiblePart = (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
		HasInvisiblePart = !HasVisiblePart || (FarthestPoint > Frustum.FarPlane || ClosestPoint < Frustum.NearPlane);
	}

	HasInvisiblePart = HasInvisiblePart || rtm::vector_any_greater_equal(CenterDistance, rtm::vector_neg(ProjectedExtent));

	return static_cast<U8>(HasVisiblePart) | (static_cast<U8>(HasInvisiblePart) << 1);
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE bool RTM_SIMD_CALL HasIntersection(rtm::vector4f_arg0 Sphere, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of sphere center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = rtm::vector_mul_add(rtm::vector_dup_x(Sphere), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_y(Sphere), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = rtm::vector_mul_add(rtm::vector_dup_z(Sphere), Frustum.LRBT_Nz, CenterDistance);

	// Plane normals look outside the frustum. Sphere.w is its radius.
	// TODO: rtm::vector_any_greater_then, or leave vector_any_greater_equal?
	if (rtm::vector_any_greater_equal(CenterDistance, rtm::vector_dup_w(Sphere)))
		return false;

	// If inside LRTB, check intersection with NF planes
	const float CenterAlongLookAxis = rtm::vector_dot3(Frustum.LookAxis, Sphere);
	const float Radius = rtm::vector_get_w(Sphere);
	const float ClosestPoint = CenterAlongLookAxis - Radius;
	const float FarthestPoint = CenterAlongLookAxis + Radius;
	return (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL ClosestPtPointAABB(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 BoxCenter, rtm::vector4f_arg2 BoxExtent) noexcept
{
	// Clamp Point to Min and Max of AABB
	return rtm::vector_min(rtm::vector_max(Point, rtm::vector_sub(BoxCenter, BoxExtent)), rtm::vector_add(BoxCenter, BoxExtent));
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE float RTM_SIMD_CALL SqMaxDistancePointAABBMinMax(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 BoxMin, rtm::vector4f_arg2 BoxMax) noexcept
{
	return rtm::vector_length_squared3(rtm::vector_max(rtm::vector_abs(rtm::vector_sub(Point, BoxMin)), rtm::vector_abs(rtm::vector_sub(Point, BoxMax))));
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE float RTM_SIMD_CALL SqMaxDistancePointAABB(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 BoxCenter, rtm::vector4f_arg2 BoxExtent) noexcept
{
	return SqMaxDistancePointAABBMinMax(Point, rtm::vector_sub(BoxCenter, BoxExtent), rtm::vector_add(BoxCenter, BoxExtent));
}
//---------------------------------------------------------------------

// Returns 0 if the point is inside
DEM_FORCE_INLINE float RTM_SIMD_CALL SqDistancePointAABBMinMax(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 BoxMin, rtm::vector4f_arg2 BoxMax) noexcept
{
	if (rtm::vector_all_greater_equal3(Point, BoxMin) && rtm::vector_all_less_equal3(Point, BoxMax))
		return 0.f;

	const auto ClosestPt = rtm::vector_min(rtm::vector_max(Point, BoxMin), BoxMax);
	return rtm::vector_length_squared3(rtm::vector_sub(Point, ClosestPt));
}
//---------------------------------------------------------------------

// Returns 0 if the point is inside
DEM_FORCE_INLINE float RTM_SIMD_CALL SqDistancePointAABB(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 BoxCenter, rtm::vector4f_arg2 BoxExtent) noexcept
{
	return SqDistancePointAABBMinMax(Point, rtm::vector_sub(BoxCenter, BoxExtent), rtm::vector_add(BoxCenter, BoxExtent));
}
//---------------------------------------------------------------------

// Returns 0 if the point is inside
DEM_FORCE_INLINE float RTM_SIMD_CALL DistancePointSphere(rtm::vector4f_arg0 Point, rtm::vector4f_arg1 Sphere) noexcept
{
	const float DistanceToCenter = rtm::vector_distance3(Point, Sphere);
	const float SphereRadius = rtm::vector_get_w(Sphere);
	return (DistanceToCenter > SphereRadius) ? (DistanceToCenter - SphereRadius) : 0.f;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE bool RTM_SIMD_CALL HasIntersection(rtm::vector4f_arg0 Sphere, rtm::vector4f_arg1 BoxCenter, rtm::vector4f_arg2 BoxExtent) noexcept
{
	const float SqDistToCenter = SqDistancePointAABB(Sphere, BoxCenter, BoxExtent);
	const float SphereRadius = rtm::vector_get_w(Sphere);
	return SqDistToCenter < SphereRadius * SphereRadius;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE bool RTM_SIMD_CALL HasIntersection(rtm::vector4f_arg0 SphereA, rtm::vector4f_arg1 SphereB) noexcept
{
	const float TotalRadius = rtm::vector_get_w(SphereA) + rtm::vector_get_w(SphereB);
	return rtm::vector_length_squared3(rtm::vector_sub(SphereA, SphereB)) <= TotalRadius * TotalRadius;
}
//---------------------------------------------------------------------

// Returns a 2 bit mask with bit0 / bit1 set if the AABB is present inside / outside the sphere.
DEM_FORCE_INLINE U8 RTM_SIMD_CALL ClipAABB(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent, rtm::vector4f_arg2 Sphere) noexcept
{
	// Calculate prerequisites
	const auto BoxMin = rtm::vector_sub(BoxCenter, BoxExtent);
	const auto BoxMax = rtm::vector_add(BoxCenter, BoxExtent);
	const float SphereRadius = rtm::vector_get_w(Sphere);

	// Check if intersection exists
	const float SqMinDistToCenter = SqDistancePointAABBMinMax(Sphere, BoxMin, BoxMax);
	if (SqMinDistToCenter >= SphereRadius * SphereRadius) return ClipOutside;

	// If intersecting, check if the box is completely inside the sphere (it is if its farthest point is)
	const float SqMaxDistToCenter = SqMaxDistancePointAABBMinMax(Sphere, BoxMin, BoxMax);
	return (SqMaxDistToCenter > SphereRadius * SphereRadius) ? ClipIntersect : ClipInside;
}
//---------------------------------------------------------------------

}
