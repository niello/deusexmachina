#pragma once
#ifndef __DEM_L1_SYS_OS_WINDOW_H__
#define __DEM_L1_SYS_OS_WINDOW_H__

// Operating system window

#if (defined __WIN32__)
#include <System/OSWindowWin32.h>
namespace Sys { class COSWindow: public COSWindowWin32 {}; }
#else
#error "COSWindow is not defined for the target OS"
#endif

namespace Sys { typedef Ptr<COSWindow> POSWindow; }

#endif
