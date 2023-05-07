#pragma once
#include <Math/Matrix44.h>
#include <System/System.h> //!!!DBG TMP! for tmp assertions, check if can be removed!
#include <acl/math/vector4_32.h>

// Math for view and projection calculations

namespace Math
{

// TODO: add an alternative implementation for AVX (ymm register can hold all 6 planes at once)
struct CSIMDFrustum
{
	// x, y and z components of Left, Right, Bottom and Top plane normals
	acl::Vector4_32Arg2 LRBT_Nx;
	acl::Vector4_32Arg3 LRBT_Ny;
	acl::Vector4_32Arg4 LRBT_Nz;

	// w component of planes, which is -d, i.e. -dot(PlaneOrigin, PlaneNormal), see CalcFrustumParams for a comment
	acl::Vector4_32Arg5 LRBT_w;

	// Separate params for near and far planes. They are considered parallel and use the same normal vector.
	acl::Vector4_32ArgN LookAxis;
	float NearPlane;
	float FarPlane;
};

// Extract frustum planes using Gribb-Hartmann method.
// See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
// See https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
// FIXME: now normals point into the frustum which may be confusing a bit. Can negate _all_ plane components to solve this, but need to fix clipping code then!
DEM_FORCE_INLINE CSIMDFrustum CalcFrustumParams(const matrix44& m) noexcept
{
	// Left = W + X, Right = W - X, Bottom = W + Y, Top = W - Y. Normals look inside the frustum, i.e. positive halfspace means 'inside'.
	// LRBT_w is a -d, i.e. -dot(PlaneNormal, PlaneOrigin). +w instead of -d allows us using a single fma instead of separate mul & sub in a box test.
	const auto LRBT_Nx = acl::vector_add(acl::vector_set(m.m[0][3]), acl::vector_set(m.m[0][0], -m.m[0][0], m.m[0][1], -m.m[0][1]));
	const auto LRBT_Ny = acl::vector_add(acl::vector_set(m.m[1][3]), acl::vector_set(m.m[1][0], -m.m[1][0], m.m[1][1], -m.m[1][1]));
	const auto LRBT_Nz = acl::vector_add(acl::vector_set(m.m[2][3]), acl::vector_set(m.m[2][0], -m.m[2][0], m.m[2][1], -m.m[2][1]));
	const auto LRBT_w =  acl::vector_add(acl::vector_set(m.m[3][3]), acl::vector_set(m.m[3][0], -m.m[3][0], m.m[3][1], -m.m[3][1]));

	// Near and far planes are parallel, which enables us to save some work. Near & far planes are +d (-w) along the look
	// axis, so NearPlane is negated once and FarPlane is negated twice (once to make it -w, once to invert the axis).
	// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W.
	// TODO: check if it is really faster than making another vector set for NF_Nx etc. At least less registers used?
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
DEM_FORCE_INLINE bool ClipAABB(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent, const CSIMDFrustum& Frustum) noexcept
{
	// FIXME: what is better, negate each box or invert frustum plane normals once?! If second, need to fix octree node culling!
	const auto BoxNegExtent = acl::vector_neg(BoxExtent);

	//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc in a loop or caching abs axes with potential store in memory?
	const auto LRBT_Abs_Nx = acl::vector_abs(Frustum.LRBT_Nx);
	const auto LRBT_Abs_Ny = acl::vector_abs(Frustum.LRBT_Ny);
	const auto LRBT_Abs_Nz = acl::vector_abs(Frustum.LRBT_Nz);

	// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
	auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(BoxCenter), Frustum.LRBT_Nx, Frustum.LRBT_w);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(BoxCenter), Frustum.LRBT_Ny, CenterDistance);
	CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(BoxCenter), Frustum.LRBT_Nz, CenterDistance);

	// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
	auto ProjectedNegExtent = acl::vector_mul(acl::vector_mix_xxxx(BoxNegExtent), LRBT_Abs_Nx);
	ProjectedNegExtent = acl::vector_mul_add(acl::vector_mix_yyyy(BoxNegExtent), LRBT_Abs_Ny, ProjectedNegExtent);
	ProjectedNegExtent = acl::vector_mul_add(acl::vector_mix_zzzz(BoxNegExtent), LRBT_Abs_Nz, ProjectedNegExtent);

	// Plane normals look inside the frustum
	if (acl::vector_any_less_equal(CenterDistance, ProjectedNegExtent))
		return false;

	//???!!!TODO PERF: rtm::vector_abs & rtm::vector_neg are bit-based! Is it better to recalc in a loop or caching abs axes with potential store in memory?
	const auto AbsLookAxis = acl::vector_abs(Frustum.LookAxis);

	// If inside LRTB, check intersection with NF planes
	const float CenterAlongLookAxis = acl::vector_dot3(Frustum.LookAxis, BoxCenter);
	const float NegExtentAlongLookAxis = acl::vector_dot3(AbsLookAxis, BoxNegExtent);
	const float ClosestPoint = CenterAlongLookAxis + NegExtentAlongLookAxis;
	const float FarthestPoint = CenterAlongLookAxis - NegExtentAlongLookAxis;

	//!!!DBG TMP! Comment when happens and is expected. Delete when fully commented.
	n_assert2_dbg(FarthestPoint > Frustum.NearPlane, "ClipAABB: Culling by the near plane happened!");
	n_assert2_dbg(ClosestPoint < Frustum.FarPlane, "ClipAABB: Culling by the far plane happened!");

	return (FarthestPoint > Frustum.NearPlane && ClosestPoint < Frustum.FarPlane);
}
//---------------------------------------------------------------------

}
