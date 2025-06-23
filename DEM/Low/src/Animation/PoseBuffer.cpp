#include "PoseBuffer.h"
#include <Math/Math.h>
#include <System/System.h>

namespace DEM::Anim
{

CPoseBuffer::CPoseBuffer(const CPoseBuffer& Other)
	: _Count(Other._Count)
{
	if (_Count)
	{
		_Transforms.reset(new rtm::qvvf[_Count]);
		n_assert_dbg(Math::IsAligned<16>(_Transforms.get()));
		std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(rtm::qvvf) * _Count);
	}
}
//---------------------------------------------------------------------

CPoseBuffer::CPoseBuffer(CPoseBuffer&& Other) noexcept
	: _Transforms(std::move(Other._Transforms))
	, _Count(Other._Count)
{
}
//---------------------------------------------------------------------

CPoseBuffer& CPoseBuffer::operator =(const CPoseBuffer& Other)
{
	SetSize(Other._Count);
	if (_Count) std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(rtm::qvvf) * _Count);
	return *this;
}
//---------------------------------------------------------------------

CPoseBuffer& CPoseBuffer::operator =(CPoseBuffer&& Other) noexcept
{
	_Count = Other._Count;
	_Transforms = std::move(Other._Transforms);
	return *this;
}
//---------------------------------------------------------------------

void CPoseBuffer::SetSize(UPTR Size)
{
	if (_Count == Size) return;
	_Count = Size;
	if (_Count)
	{
		_Transforms.reset(new rtm::qvvf[_Count]);
		for (auto It = begin(); It != end(); ++It)
			*It = rtm::qvv_identity();
	}
	else
	{
		_Transforms.reset();
	}
	n_assert_dbg(Math::IsAligned<16>(_Transforms.get()));
}
//---------------------------------------------------------------------

}
