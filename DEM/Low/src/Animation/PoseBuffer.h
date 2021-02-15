#pragma once
#include <StdDEM.h>
#include <acl/math/transform_32.h>

// Buffer with per-bone transforms of the whole skeleton

namespace DEM::Anim
{

class CPoseBuffer final
{
protected:

	std::unique_ptr<acl::Transform_32[]> _Transforms;
	UPTR                                 _Count = 0;

public:

	CPoseBuffer() = default;
	CPoseBuffer(const CPoseBuffer& Other);
	CPoseBuffer(CPoseBuffer&& Other) noexcept;

	CPoseBuffer& operator =(const CPoseBuffer& Other);
	CPoseBuffer& operator =(CPoseBuffer&& Other) noexcept;

	void SetSize(UPTR Size);
	UPTR size() const { return _Count; }
	auto begin() { return _Transforms.get(); }
	auto begin() const { return _Transforms.get(); }
	auto end() { return _Transforms.get() + _Count; }
	auto end() const { return _Transforms.get() + _Count; }

	auto& operator [](UPTR Index) { return _Transforms.get()[Index]; }
	auto& operator [](UPTR Index) const { return _Transforms.get()[Index]; }

	void NormalizeRotations()
	{
		for (auto It = begin(); It != end(); ++It)
			It->rotation = acl::quat_normalize(It->rotation);
	}

	void Accumulate(const CPoseBuffer& Other)
	{
		const UPTR Size = std::min(_Count, Other._Count);
		for (UPTR i = 0; i < Size; ++i)
		{
			const auto& OtherTfm = Other[i];
			auto& Tfm = _Transforms.get()[i];

			// Blend with shortest arc, based on a 4D dot product sign
			if (acl::vector_dot(Tfm.rotation, OtherTfm.rotation) < 0.f)
				Tfm.rotation = acl::vector_sub(Tfm.rotation, OtherTfm.rotation);
			else
				Tfm.rotation = acl::vector_add(Tfm.rotation, OtherTfm.rotation);

			Tfm.translation = acl::vector_add(Tfm.translation, OtherTfm.translation);
			Tfm.scale = acl::vector_add(Tfm.scale, OtherTfm.scale);
		}
	}

	void Accumulate(const CPoseBuffer& Other, float Weight)
	{
		const auto WeightVector = acl::vector_set(Weight);
		const UPTR Size = std::min(_Count, Other._Count);
		for (UPTR i = 0; i < Size; ++i)
		{
			const auto& OtherTfm = Other[i];
			auto& Tfm = _Transforms.get()[i];

			// Blend with shortest arc, based on a 4D dot product sign
			auto OtherRotation = OtherTfm.rotation;
			if (acl::vector_dot(Tfm.rotation, OtherRotation) < 0.f)
				OtherRotation = acl::vector_neg(OtherRotation);
			Tfm.rotation = acl::vector_mul_add(OtherRotation, WeightVector, Tfm.rotation);

			Tfm.translation = acl::vector_mul_add(OtherTfm.translation, WeightVector, Tfm.translation);
			Tfm.scale = acl::vector_mul_add(OtherTfm.scale, WeightVector, Tfm.scale);
		}
	}

	void operator *=(float Weight)
	{
		const auto WeightVector = acl::vector_set(Weight);
		for (UPTR i = 0; i < _Count; ++i)
		{
			auto& Tfm = _Transforms.get()[i];
			Tfm.scale = acl::vector_mul(Tfm.scale, WeightVector);
			Tfm.rotation = acl::vector_mul(Tfm.rotation, WeightVector);
			Tfm.translation = acl::vector_mul(Tfm.translation, WeightVector);
		}
	}
};

}
