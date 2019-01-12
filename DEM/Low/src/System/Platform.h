#pragma once
#include <Data/Ptr.h>

// Platform-dependent functionality interface. Implemented per-platform / OS.

namespace DEM { namespace Sys
{
typedef Ptr<class COSWindow> POSWindow;
class IOSFileSystem;

enum ESystemFolder
{
	SysFolder_Bin,			// Application executable directory
	SysFolder_Home,			// Application home directory
	SysFolder_WorkingDir,	// Application working directory
	SysFolder_User,			// OS user documents folder
	SysFolder_AppData,		// OS location for app's read-write data
	SysFolder_Temp,			// OS temporary files location
	SysFolder_Programs		// OS folder where applications are installed
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
