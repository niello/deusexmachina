#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include "acl/core/compiler_utils.h"
#include "acl/math/math.h"
#include "acl/math/quat_64.h"
#include "acl/math/vector4_64.h"
#include "acl/math/affine_matrix_64.h"

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	constexpr Transform_64 transform_set(const Quat_64& rotation, const Vector4_64& translation, const Vector4_64& scale)
	{
		return Transform_64{ rotation, translation, scale };
	}

	inline Transform_64 transform_identity_64()
	{
		return transform_set(quat_identity_64(), vector_zero_64(), vector_set(1.0));
	}

	inline Transform_64 transform_cast(const Transform_32& input)
	{
		return Transform_64{ quat_cast(input.rotation), vector_cast(input.translation), vector_cast(input.scale) };
	}

	// Multiplication order is as follow: local_to_world = transform_mul(local_to_object, object_to_world)
	// NOTE: When scale is present, multiplication will not properly handle skew/shear, use affine matrices instead
	inline Transform_64 transform_mul(const Transform_64& lhs, const Transform_64& rhs)
	{
		const Vector4_64 min_scale = vector_min(lhs.scale, rhs.scale);
		const Vector4_64 scale = vector_mul(lhs.scale, rhs.scale);

		if (vector_any_less_than3(min_scale, vector_zero_64()))
		{
			// If we have negative scale, we go through a matrix
			const AffineMatrix_64 lhs_mtx = matrix_from_transform(lhs);
			const AffineMatrix_64 rhs_mtx = matrix_from_transform(rhs);
			AffineMatrix_64 result_mtx = matrix_mul(lhs_mtx, rhs_mtx);
			result_mtx = matrix_remove_scale(result_mtx);

			const Vector4_64 sign = vector_sign(scale);
			result_mtx.x_axis = vector_mul(result_mtx.x_axis, vector_mix_xxxx(sign));
			result_mtx.y_axis = vector_mul(result_mtx.y_axis, vector_mix_yyyy(sign));
			result_mtx.z_axis = vector_mul(result_mtx.z_axis, vector_mix_zzzz(sign));

			const Quat_64 rotation = quat_from_matrix(result_mtx);
			const Vector4_64 translation = result_mtx.w_axis;
			return transform_set(rotation, translation, scale);
		}
		else
		{
			const Quat_64 rotation = quat_mul(lhs.rotation, rhs.rotation);
			const Vector4_64 translation = vector_add(quat_rotate(rhs.rotation, vector_mul(lhs.translation, rhs.scale)), rhs.translation);
			return transform_set(rotation, translation, scale);
		}
	}

	// Multiplication order is as follow: local_to_world = transform_mul(local_to_object, object_to_world)
	inline Transform_64 transform_mul_no_scale(const Transform_64& lhs, const Transform_64& rhs)
	{
		const Quat_64 rotation = quat_mul(lhs.rotation, rhs.rotation);
		const Vector4_64 translation = vector_add(quat_rotate(rhs.rotation, lhs.translation), rhs.translation);
		return transform_set(rotation, translation, vector_set(1.0));
	}

	inline Vector4_64 transform_position(const Transform_64& lhs, const Vector4_64& rhs)
	{
		return vector_add(quat_rotate(lhs.rotation, vector_mul(lhs.scale, rhs)), lhs.translation);
	}

	inline Vector4_64 transform_position_no_scale(const Transform_64& lhs, const Vector4_64& rhs)
	{
		return vector_add(quat_rotate(lhs.rotation, rhs), lhs.translation);
	}

	inline Transform_64 transform_inverse(const Transform_64& input)
	{
		const Quat_64 inv_rotation = quat_conjugate(input.rotation);
		const Vector4_64 inv_scale = vector_reciprocal(input.scale);
		const Vector4_64 inv_translation = vector_neg(quat_rotate(inv_rotation, vector_mul(input.translation, inv_scale)));
		return transform_set(inv_rotation, inv_translation, inv_scale);
	}

	inline Transform_64 transform_inverse_no_scale(const Transform_64& input)
	{
		const Quat_64 inv_rotation = quat_conjugate(input.rotation);
		const Vector4_64 inv_translation = vector_neg(quat_rotate(inv_rotation, input.translation));
		return transform_set(inv_rotation, inv_translation, vector_set(1.0));
	}

	inline Transform_64 transform_normalize(const Transform_64& input)
	{
		const Quat_64 rotation = quat_normalize(input.rotation);
		return transform_set(rotation, input.translation, input.scale);
	}
}

ACL_IMPL_FILE_PRAGMA_POP
