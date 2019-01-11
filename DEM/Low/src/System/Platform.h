#pragma once
#include <Data/Ptr.h>

// Platform-dependent functionality interface. Implemented per-platform / OS.

namespace DEM { namespace Sys
{
typedef Ptr<class COSWindow> POSWindow;
class IOSFileSystem;

enum ESystemFolder
{
	SysFolder_User,		// OS user docs folder. Used for saves and configs so must have write access.
	SysFolder_Home,		// Application root directory
	SysFolder_Temp,		// OS temporary files location
	SysFolder_Bin,		// Application executable directory
	SysFolder_AppData,	// Application-wide read-write data location
	SysFolder_Programs	// OS folder where applications are installed
};

class IPlatform
{
public:

	virtual double GetSystemTime() const = 0; // In seconds

	virtual POSWindow CreateGUIWindow() = 0;
	//virtual POSConsoleWindow CreateConsoleWindow() = 0;

	virtual IOSFileSystem* GetFileSystemInterface() const = 0;
	virtual bool GetSystemFolderPath(ESystemFolder Code, CString& OutPath) const = 0;

	virtual void Update() = 0;
};

}
};
