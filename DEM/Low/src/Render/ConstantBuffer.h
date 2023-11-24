#pragma once
#ifndef __DEM_L1_RENDER_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_CONSTANT_BUFFER_H__

#include <Core/Object.h>

// Storage that contains shader uniform constant values.
// For modern APIs it is implemented as a hardware buffer.
// For older APIs like DX9 a buffer is emulated in RAM.

namespace Render
{

class CConstantBuffer: public Core::CObject
{
public:

	virtual bool IsValid() const = 0;
	virtual bool IsInWriteMode() const = 0;
	virtual bool IsDirty() const = 0;
	virtual bool IsTemporary() const = 0;
	virtual U8   GetAccessFlags() const = 0;
	virtual void SetDebugName(std::string_view Name) = 0;
};

typedef Ptr<CConstantBuffer> PConstantBuffer;

}

#endif
