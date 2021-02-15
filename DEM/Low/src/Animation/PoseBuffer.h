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

	void operator *=(float Weight)
	{
		const auto WeightVector = acl::vector_set(Weight);
		for (UPTR i = 0; i < _Count; ++i)
		{
			auto& Tfm = _Transforms.get()[i];
			acl::vector_mul(Tfm.scale, WeightVector);
			acl::vector_mul(Tfm.rotation, WeightVector);
			acl::vector_mul(Tfm.translation, WeightVector);
		}
	}
};

}
