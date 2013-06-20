#pragma once
#ifndef __DEM_TOOLS_CONSOLE_APP_H__
#define __DEM_TOOLS_CONSOLE_APP_H__

#include <ncmdlineargs.h>
#include <conio.h>

#define SEP_LINE "--------------------------------------------------------------------------------"

#define n_msg(Verbosity, String, ...) { if (Verbose >= Verbosity) n_printf(String, __VA_ARGS__); }
#define VR_ALWAYS	0
#define VR_ERROR	1
#define VR_WARNING	2
#define VR_INFO		3
#define VR_DETAILS	4
#define VR_DEBUG	5

//???to console application class?
extern int Verbose;

#endif
