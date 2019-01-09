#pragma once
#include <Data/Ptr.h>

// Platform-dependent functionality interface. Implemented per-platform / OS.

namespace DEM { namespace Sys
{
typedef Ptr<class COSWindow> POSWindow;

class IPlatform
{
public:

	virtual double GetSystemTime() const = 0; // In seconds

	virtual POSWindow CreateGUIWindow() = 0;
	//virtual POSConsoleWindow CreateConsoleWindow() = 0;

	virtual void Update() = 0;
};

}
};
