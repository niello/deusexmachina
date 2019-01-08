#pragma once

#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/Params.h>
#include <Data/HashTable.h>
#include <Data/Dictionary.h>

// DEM application base class

//???use CApplicationWin32 : public CApplicationBase + typedef CApplicationWin32 CApplication; ?

// OS-specific:
// - file IO
// - time
// - memory
// - window (system GUI & input)
// - threads

// DEM:
// - app settings
// - global variables
// - events
// - user profiles (settings & saves)
// - time sources (need named timers? use delayed events with handles?)
// - application states (FSM)
// - callbacks / virtual methods for application lifecycle control in derived applications
// - factory

namespace DEM { namespace Core
{

class CApplication
{
protected:

	//

public:

	//

	void Start();
};

}};
