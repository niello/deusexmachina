#pragma once
#ifndef __DEM_L1_SYS_OS_WINDOW_CLASS_H__
#define __DEM_L1_SYS_OS_WINDOW_CLASS_H__

// Operating system window class. Window class is a template for possibly
// multiple window instances. Store shared properties here.

#if (defined __WIN32__)
#include <System/OSWindowClassWin32.h>
namespace Sys { class COSWindowClass: public COSWindowClassWin32 {}; }
#else
#error "COSWindowClass is not defined for the target OS"
#endif

namespace Sys { typedef Ptr<class COSWindowClass> POSWindowClass; }

#endif
