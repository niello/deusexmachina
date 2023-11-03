#pragma once
#include <Math/Matrix44.h>
#include <System/System.h> //!!!DBG TMP! for tmp assertions, check if can be removed!
#include <acl/math/vector4_32.h>

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
	acl::Vector4_32 LRBT_Nx;
	acl::Vector4_32 LRBT_Ny;
	acl::Vector4_32 LRBT_Nz;

	// w component of planes, which is -d, i.e. -dot(PlaneOrigin, PlaneNormal), see CalcFrustumParams for a comment
	acl::Vector4_32 LRBT_w;

	// Separate params for near and far planes. They are considered parallel and use the same normal vector.
	acl::Vector4_32 LookAxis;
	float NearPlane;
	float FarPlane;
};

// Extract frustum planes using Gribb-Hartmann method.
// See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
// See https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
DEM_FORCE_INLINE CSIMDFrustum CalcFrustumParams(const matrix44& m) noexcept
{
	// In the paper Left = W + X, Right = W - X, Bottom = W + Y, Top = W - Y. Normals look inside the frustum, i.e. positive halfspace means 'inside'.
	// We invert this to Left = -X - W, Right = X - W, Bottom = -Y - W, Top = Y - W. Normals look outside the frustum, i.e. positive halfspace means 'outside'.
	// LRBT_w is a -d, i.e. -dot(PlaneNormal, PlaneOrigin). +w instead of -d allows us using a single fma instead of separate mul & sub in clipping tests.
	const auto LRBT_Nx = acl::vector_sub(acl::vector_set(-m.m[0][0], m.m[0][0], -m.m[0][1], m.m[0][1]), acl::vector_set(m.m[0][3]));
	const auto LRBT_Ny = acl::vector_sub(acl::vector_set(-m.m[1][0], m.m[1][0], -m.m[1][1], m.m[1][1]), acl::vector_set(m.m[1][3]));
	const auto LRBT_Nz = acl::vector_sub(acl::vector_set(-m.m[2][0], m.m[2][0], -m.m[2][1], m.m[2][1]), acl::vector_set(m.m[2][3]));
	const auto LRBT_w =  acl::vector_sub(acl::vector_set(-m.m[3][0], m.m[3][0], -m.m[3][1], m.m[3][1]), acl::vector_set(m.m[3][3]));

	// Near and far planes are parallel, which enables us to save some work. Near & far planes are +d (-w) along the look
	// axis, so NearPlane is negated once and FarPlane is negated twice (once to make it -w, once to invert the axis).
	// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W.
	// TODO: check if it is really faster than making another vector set for NF_Nx etc. At least less SSE registers used?
	const auto NearAxis = acl::vector_set(m.m[0][2], m.m[1][2], m.m[2][2], 0.f);
	const auto FarAxis = acl::vector_sub(acl::vector_set(m.m[0][3], m.m[1][3], m.m[2][3], 0.f), NearAxis);
	const float InvNearLen = acl::sqrt_reciprocal(acl::vector_length_squared3(NearAxis));
	const auto LookAxis = acl::vector_mul(NearAxis, InvNearLen);
	const float NearPlane = -m.m[3][2] * InvNearLen;
	const float FarPlane = (m.m[3][3] - m.m[3][2]) * acl::sqrt_reciprocal(acl::vector_length_squared3(FarAxis));

	return { LRBT_Nx, LRBT_Ny, LRBT_Nz, LRBT_w, LookAxis, NearPlane, FarPlane };
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE bool HasIntersection(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(BoxCenter), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(BoxCenter), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(BoxCenter), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
	auto ProjectedExtent = acl::vector_mul(acl::vector_mix_xxxx(BoxExtent), acl::vector_abs(Frustum.LRBT_Nx));
	ProjectedExtent = acl::vector_mul_add(acl::vector_mix_yyyy(BoxExtent), acl::vector_abs(Frustum.LRBT_Ny), ProjectedExtent);
	ProjectedExtent = acl::vector_mul_add(acl::vector_mix_zzzz(BoxExtent), acl::vector_abs(Frustum.LRBT_Nz), ProjectedExtent);

	// Plane normals look outside the frustum
	// TODO: rtm::vector_any_greater_then, or leave vector_any_greater_equal?
	if (acl::vector_any_greater_equal(CenterDistance, ProjectedExtent))
		return false;

	// If inside LRTB, check intersection with NF planes
	const float CenterAlongLookAxis = acl::vector_dot3(Frustum.LookAxis, BoxCenter);
	const float ExtentAlongLookAxis = acl::vector_dot3(acl::vector_abs(Frustum.LookAxis), BoxExtent);
	const float ClosestPoint = CenterAlongLookAxis - ExtentAlongLookAxis;
	const float FarthestPoint = CenterAlongLookAxis + ExtentAlongLookAxis;
	return (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// Returns a 2 bit mask with bit0 set if the AABB is present inside and bit1 if outside the frustum.
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE U8 ClipAABB(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(BoxCenter), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(BoxCenter), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(BoxCenter), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
	auto ProjectedExtent = acl::vector_mul(acl::vector_mix_xxxx(BoxExtent), acl::vector_abs(Frustum.LRBT_Nx));
	ProjectedExtent = acl::vector_mul_add(acl::vector_mix_yyyy(BoxExtent), acl::vector_abs(Frustum.LRBT_Ny), ProjectedExtent);
	ProjectedExtent = acl::vector_mul_add(acl::vector_mix_zzzz(BoxExtent), acl::vector_abs(Frustum.LRBT_Nz), ProjectedExtent);

	// Plane normals look outside the frustum
	bool HasVisiblePart = acl::vector_all_less_equal(CenterDistance, ProjectedExtent);
	bool HasInvisiblePart = false;
	if (HasVisiblePart)
	{
		// If inside LRTB, check intersection with NF planes
		const float CenterAlongLookAxis = acl::vector_dot3(Frustum.LookAxis, BoxCenter);
		const float ExtentAlongLookAxis = acl::vector_dot3(acl::vector_abs(Frustum.LookAxis), BoxExtent);
		const float ClosestPoint = CenterAlongLookAxis - ExtentAlongLookAxis;
		const float FarthestPoint = CenterAlongLookAxis + ExtentAlongLookAxis;
		HasVisiblePart = (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
		HasInvisiblePart = !HasVisiblePart || (FarthestPoint > Frustum.FarPlane || ClosestPoint < Frustum.NearPlane);
	}

	HasInvisiblePart = HasInvisiblePart || acl::vector_any_greater_equal(CenterDistance, acl::vector_neg(ProjectedExtent));

	return static_cast<U8>(HasVisiblePart) | (static_cast<U8>(HasInvisiblePart) << 1);
}
//---------------------------------------------------------------------

// Test AABB vs frustum planes intersection for positive halfspace treated as 'inside'
// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc abs axes or cache them in CSIMDFrustum (probably in memory)?
DEM_FORCE_INLINE bool HasIntersection(acl::Vector4_32Arg0 Sphere, const CSIMDFrustum& Frustum) noexcept
{
	// Distance of sphere center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(Sphere), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(Sphere), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(Sphere), Frustum.LRBT_Nz, CenterDistance);

	// Plane normals look outside the frustum. Sphere.w is its radius.
	// TODO: rtm::vector_any_greater_then, or leave vector_any_greater_equal?
	if (acl::vector_any_greater_equal(CenterDistance, acl::vector_mix_wwww(Sphere)))
		return false;

	// If inside LRTB, check intersection with NF planes
	const float CenterAlongLookAxis = acl::vector_dot3(Frustum.LookAxis, Sphere);
	const float Radius = acl::vector_get_w(Sphere);
	const float ClosestPoint = CenterAlongLookAxis - Radius;
	const float FarthestPoint = CenterAlongLookAxis + Radius;
	return (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE acl::Vector4_32 ClosestPtPointAABB(acl::Vector4_32Arg0 Point, acl::Vector4_32Arg1 BoxCenter, acl::Vector4_32Arg2 BoxExtent) noexcept
{
	// Clamp Point to Min and Max of AABB
	return acl::vector_min(acl::vector_max(Point, acl::vector_sub(BoxCenter, BoxExtent)), acl::vector_add(BoxCenter, BoxExtent));
}
//---------------------------------------------------------------------

// Returns 0 if the point is inside
DEM_FORCE_INLINE float SqDistancePointAABB(acl::Vector4_32Arg0 Point, acl::Vector4_32Arg1 BoxCenter, acl::Vector4_32Arg2 BoxExtent) noexcept
{
	const auto BoxMin = acl::vector_sub(BoxCenter, BoxExtent);
	const auto BoxMax = acl::vector_add(BoxCenter, BoxExtent);

	if (acl::vector_all_greater_equal3(Point, BoxMin) && acl::vector_all_less_equal3(Point, BoxMax))
		return 0.f;

	const auto ClosestPt = acl::vector_min(acl::vector_max(Point, BoxMin), BoxMax);
	return acl::vector_length_squared3(acl::vector_sub(Point, ClosestPt));
}
//---------------------------------------------------------------------

// Returns 0 if the point is inside
DEM_FORCE_INLINE float DistancePointSphere(acl::Vector4_32Arg0 Point, acl::Vector4_32Arg1 Sphere) noexcept
{
	const float DistanceToCenter = acl::vector_distance3(Point, Sphere);
	const float SphereRadius = acl::vector_get_w(Sphere);
	return (DistanceToCenter > SphereRadius) ? (DistanceToCenter - SphereRadius) : 0.f;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE bool HasIntersection(acl::Vector4_32Arg0 Sphere, acl::Vector4_32Arg1 BoxCenter, acl::Vector4_32Arg2 BoxExtent) noexcept
{
	const float SqDistToCenter = SqDistancePointAABB(Sphere, BoxCenter, BoxExtent);
	const float SphereRadius = acl::vector_get_w(Sphere);
	return SqDistToCenter <= SphereRadius * SphereRadius;
}
//---------------------------------------------------------------------

}
