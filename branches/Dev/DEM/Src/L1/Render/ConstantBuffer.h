#pragma once
#ifndef __DEM_L1_RENDER_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_CONSTANT_BUFFER_H__

#include <Core/Object.h>

// A hardware GPU buffer that contains shader uniform constants

namespace Render
{

class CConstantBuffer: public Core::CObject
{
protected:

public:

	// set value to some offset/register

	virtual void	Destroy() = 0;
	virtual bool	IsValid() const = 0;
};

typedef Ptr<CConstantBuffer> PConstantBuffer;

}

#endif
