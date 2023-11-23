#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// A hardware GPU buffer that stores indices in a corresponding array of vertices

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CIndexBuffer: public Core::CObject
{
	//RTTI_CLASS_DECL;

protected:

	EIndexType		IndexType = EIndexType::Index_16;
	UPTR			IndexCount = 0;
	Data::CFlags	Access;

	void InternalDestroy() { IndexCount = 0; Access.ClearAll(); }

public:

	virtual ~CIndexBuffer() { InternalDestroy(); }

	virtual void	Destroy() { InternalDestroy(); }
	virtual bool	IsValid() const = 0;
	virtual void    SetDebugName(std::string_view Name) = 0;

	Data::CFlags	GetAccess() const { return Access; }
	EIndexType		GetIndexType() const { return IndexType; }
	UPTR			GetIndexCount() const { return IndexCount; }
	UPTR			GetSizeInBytes() const { return IndexCount * (UPTR)IndexType; }
};

typedef Ptr<CIndexBuffer> PIndexBuffer;

}
