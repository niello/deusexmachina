//------------------------------------------------------------------------------
//  nstartupchecker.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "util/nstartupchecker.h"
#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <d3d9.h>
#include <mmsystem.h>
#include <dsound.h>
#endif

//!!!DUPLICATE, see CWin32Display!
#define NEBULA2_WINDOW_CLASS "Nebula2::MainWindow"

//------------------------------------------------------------------------------
/**
*/
nStartupChecker::nStartupChecker()
#ifdef __WIN32__
    : globalMutex(0)
#endif
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nStartupChecker::~nStartupChecker()
{
#ifdef __WIN32__
    // release the global mutex (used for the run-once-check)
    if (0 != this->globalMutex)
    {
        CloseHandle(this->globalMutex);
        this->globalMutex = 0;
    }
#endif
}

//------------------------------------------------------------------------------
/**
    This method checks if the application is already running by creating
    a global mutex, constructed from the vendor and application name. If
    the mutex already exists, the method will return true, indicating that
    an instance of the application is already running. The mutex will be
    destroyed in the destructor of nStartupChecker, thus it is necessary
    to keep the nStartupChecker object alive for the runtime of the
    application.

    If the application is already running it will do a FindWindow() on the
    provided window name, and if successful, bring the window to front.
    If not, the application is running in a different desktop session and
    in this case, the provided error message will be displayed to the user.

    @param vendorName       the vendor name, used to construct the global mutex name
    @param appName          the application name, used to construct the global mutex name
    @param appWindowTitle   the app window's title, used for FindWindow()
    @param errorTitle       title of the error message box if app is running in different session
    @param errorMsg         the error message itself
*/
bool
nStartupChecker::CheckAlreadyRunning(const nString& vendorName,
                                     const nString& appName,
                                     const nString& appWindowTitle,
                                     const nString& errorTitle,
                                     const nString& errorMsg)
{
#ifdef __WIN32__
    n_assert(0 == this->globalMutex);

    // construct a mutex name from the vendor and app name
    nString mutexName = vendorName + "_" + appName;

    // create the mutex
    this->globalMutex = CreateMutex(0, TRUE, mutexName.Get());

    // check if the mutex already existed
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        // the app was already running, try to activate its window
        // NOTE: the window class name must be the same as the one
        // that is used in nwind32windowhandler.cc, but since the
        // graphics subsystem doesn't exist yet, we cannot ask it for the name!
        HWND hwnd = FindWindow(NEBULA2_WINDOW_CLASS, appWindowTitle.Get());
        if (0 != hwnd)
        {
            // bring the existing app window to the foreground
            SetForegroundWindow(hwnd);
            if (IsIconic(hwnd))
            {
                ShowWindow(hwnd, SW_RESTORE);
            }
            return true;
        }
        // app is probably running in a different session, show a message box
        MessageBox(0, errorMsg.Get(), errorTitle.Get(), MB_OK | MB_ICONERROR);
        return true;
    }
    // the app was not already running
    return false;
#else
    return false;
#endif
}

#if defined(__WIN32__) || defined(DOXYGEN)
//------------------------------------------------------------------------------
/**
    This method checks if D3D can be opened at all. If D3D can not be opened,
    a message box will be displayed with the provided error message.

    @param errorTitle   the title used for the error message box
    @param errorMsg     the error message, displayed in the message box
*/
bool
nStartupChecker::CheckDirect3D(const nString& errorTitle, const nString& errorMsg)
{
    IDirect3D9* pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (pD3D9)
    {
        pD3D9->Release();
        return true;
    }
    // display error message box
    MessageBox(0, errorMsg.Get(), errorTitle.Get(), MB_OK | MB_ICONERROR);
    return false;
}
#endif

#if defined(__WIN32__) || defined(DOXYGEN)
//------------------------------------------------------------------------------
/**
    This method checks if DirectSound can be opened at all. If not, an
    error message box will be displayed.

    @param errorTitle   the title used for the error message box
    @param errorMsg     the error message, displayed in the message box
*/
bool
nStartupChecker::CheckDirectSound(const nString& errorTitle, const nString& errorMsg)
{
    LPDIRECTSOUND8 ds8;
    HRESULT hr = DirectSoundCreate8(NULL, &ds8, NULL);
    if (SUCCEEDED(hr))
    {
        ds8->Release();
        return true;
    }
    // display error message box
    MessageBox(0, errorMsg.Get(), errorTitle.Get(), MB_OK | MB_ICONERROR);
    return false;
}
#endif
