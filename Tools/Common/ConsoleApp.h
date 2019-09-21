#pragma once
#ifndef __DEM_TOOLS_CONSOLE_APP_H__
#define __DEM_TOOLS_CONSOLE_APP_H__

#include <CmdLineArgs.h>
#include <conio.h>

#define API extern "C" __declspec(dllexport)

#define MAX_CMDLINE_CHARS 32000

// Return codes
#define SUCCESS						0
#define SUCCESS_HELP				100
#define ERR_MAIN_FAILED				1
#define ERR_IN_OUT_TYPES_DONT_MATCH 2
#define ERR_IN_NOT_FOUND			3
#define ERR_NOT_IMPLEMENTED_YET		4
#define ERR_INVALID_CMD_LINE		5
#define ERR_IO_READ					6
#define ERR_IO_WRITE				7
#define ERR_INVALID_DATA			8

#define SEP_LINE "------------------------------------------------------------------------------\n"

// Verbosity levels
#define VL_ALWAYS	0	// Message is printed always
#define VL_ERROR	1
#define VL_WARNING	2
#define VL_INFO		3
#define VL_DETAILS	4
#define VL_DEBUG	5

#define n_msg(Verbosity, String, ...) { if (Verbose >= Verbosity) Sys::Log(String, __VA_ARGS__); }

//???to console application class?
extern int Verbose;

#endif
