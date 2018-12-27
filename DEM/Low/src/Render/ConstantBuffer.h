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

	virtual void	Destroy() = 0;
	virtual bool	IsValid() const = 0;
};

typedef Ptr<CConstantBuffer> PConstantBuffer;

}

#endif
