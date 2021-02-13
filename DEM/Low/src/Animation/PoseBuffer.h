#pragma once
#include <StdDEM.h>
//#include <acl/math/transform_32.h>
#include <Math/TransformSRT.h>

// Buffer with per-bone transforms of the whole skeleton

namespace DEM::Anim
{

class CPoseBuffer final
{
protected:

	//std::unique_ptr<acl::Transform_32[]> _Transforms;
	std::unique_ptr<Math::CTransform> _Transforms;
	UPTR                              _Count = 0;

public:

	CPoseBuffer() = default;
	CPoseBuffer(const CPoseBuffer& Other);
	CPoseBuffer(CPoseBuffer&& Other);

	CPoseBuffer& operator =(const CPoseBuffer& Other);
	CPoseBuffer& operator =(CPoseBuffer&& Other);

	void SetSize(UPTR Size);

	auto& operator [](UPTR Index) { return _Transforms.get()[Index]; }
	auto& operator [](UPTR Index) const { return _Transforms.get()[Index]; }

	void Accumulate(const CPoseBuffer& Other)
	{
		const UPTR Size = std::min(_Count, Other._Count);
		for (UPTR i = 0; i < Size; ++i)
			_Transforms.get()[i].Accumulate(Other[i]);
	}

	void operator *=(float Weight)
	{
		for (UPTR i = 0; i < _Count; ++i)
			_Transforms.get()[i] *= Weight;
	}
};

}
