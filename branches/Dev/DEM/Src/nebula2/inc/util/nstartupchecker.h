#ifndef UTIL_STARTUPCHECKER_H
#define UTIL_STARTUPCHECKER_H
//------------------------------------------------------------------------------
/**
    @class nStartupChecker
    @ingroup Util

    The nStartupChecker class checks if the host system meets all
    preconditions to start the application (correct D3D version,
    working sound, run-once check). There should be exactly one nStartupChecker
    object alive while the application is running.

    (C) 2005 Radon Labs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nstring.h"
#if defined(__WIN32__)
typedef void *HANDLE;
#endif

//------------------------------------------------------------------------------
class nStartupChecker
{
public:
    /// constructor
    nStartupChecker();
    /// destructor
    ~nStartupChecker();
    /// check if the app is already running
    bool CheckAlreadyRunning(const nString& vendorName, const nString& appName, const nString& appWindowTitle, const nString& errorTitle, const nString& errorMsg);
#if defined(__WIN32__) || defined(DOXYGEN)
    /// check for correct Direct3D version
    bool CheckDirect3D(const nString& errorTitle, const nString& errorMsg);
    /// check for working sound
    bool CheckDirectSound(const nString& errorTitle, const nString& errorMsg);
#endif

private:
#if defined(__WIN32__) || defined(DOXYGEN)
    HANDLE globalMutex;         // required for the check whether the app is already running
#endif
};
//------------------------------------------------------------------------------
#endif
