#include "PoseBuffer.h"
#include <System/System.h>

namespace DEM::Anim
{

CPoseBuffer::CPoseBuffer(const CPoseBuffer& Other)
	: _Count(Other._Count)
{
	if (_Count)
	{
		_Transforms.reset(new acl::Transform_32[_Count]);
		n_assert_dbg(IsAligned16(_Transforms.get()));
		std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(acl::Transform_32) * _Count);
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
	if (_Count) std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(acl::Transform_32) * _Count);
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
	if (_Count) _Transforms.reset(new acl::Transform_32[_Count]);
	else _Transforms.reset();
	n_assert_dbg(IsAligned16(_Transforms.get()));
}
//---------------------------------------------------------------------

}
