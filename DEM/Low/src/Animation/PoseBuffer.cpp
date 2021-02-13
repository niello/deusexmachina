#include "PoseBuffer.h"

namespace DEM::Anim
{

CPoseBuffer::CPoseBuffer(const CPoseBuffer& Other)
	: _Count(Other._Count)
{
	if (_Count)
	{
		_Transforms.reset(new Math::CTransform[_Count]);
		std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(Math::CTransform) * _Count);
	}
}
//---------------------------------------------------------------------

CPoseBuffer::CPoseBuffer(CPoseBuffer&& Other)
	: _Transforms(std::move(Other._Transforms))
	, _Count(Other._Count)
{
}
//---------------------------------------------------------------------

CPoseBuffer& CPoseBuffer::operator =(const CPoseBuffer& Other)
{
	_Count = Other._Count;
	if (_Count)
	{
		_Transforms.reset(new Math::CTransform[_Count]);
		std::memcpy(_Transforms.get(), Other._Transforms.get(), sizeof(Math::CTransform) * _Count);
	}
	return *this;
}
//---------------------------------------------------------------------

CPoseBuffer& CPoseBuffer::operator =(CPoseBuffer&& Other)
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
	//if (_Count) _Transforms.reset(new acl::Transform_32[_Count]);
	if (_Count) _Transforms.reset(new Math::CTransform[_Count]);
	else _Transforms.reset();
}
//---------------------------------------------------------------------

}
