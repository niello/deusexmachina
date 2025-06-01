#pragma once
#include <Core/Object.h>

// A hardware GPU fence that can be signaled from GPU and waited for on CPU

namespace Render
{

class IGPUFence : public DEM::Core::CObject
{
public:

	virtual bool IsSignaled() const = 0;
	virtual void Wait() = 0;
	//virtual void WaitFor(time) = 0;
};

using PGPUFence = Ptr<IGPUFence>;

}
