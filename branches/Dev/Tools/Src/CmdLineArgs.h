#pragma once
#ifndef __DEM_TOOLS_CMD_LINE_ARGS_H__
#define __DEM_TOOLS_CMD_LINE_ARGS_H__

#include <stdlib.h>
#include <math.h>
#include <Data/String.h>

// Helper class to extract arguments from a ANSI-C command line.
// (C) 2003 RadonLabs GmbH

class nCmdLineArgs
{
private:

	int				argCount;
	const char**	argVector;

	int FindArg(const CString& option) const;

public:

	nCmdLineArgs(): argCount(0), argVector(NULL) {}
	nCmdLineArgs(int argc, const char** argv): argCount(argc), argVector(argv) {}

	void	Initialize(int argc, const char** argv) { argCount = argc; argVector = argv; }
	float	GetFloatArg(const CString& option, float defaultValue = 0.0f) const;
	int		GetIntArg(const CString& option, int defaultValue = 0) const;
	bool	GetBoolArg(const CString& option) const { return FindArg(option) > 0; }
	CString	GetStringArg(const CString& option, const CString& defaultValue = NULL) const;
	bool	HasArg(const CString& option) const { return FindArg(option) != 0; }
};

inline int nCmdLineArgs::FindArg(const CString& option) const
{
	for (int i = 0; i < argCount; i++)
		if (option == argVector[i])
			return i;
	return -1;
}
//---------------------------------------------------------------------

inline float nCmdLineArgs::GetFloatArg(const CString& option, float defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return (float)atof(argVector[i]);
	else return defaultValue;
}
//---------------------------------------------------------------------

inline int nCmdLineArgs::GetIntArg(const CString& option, int defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return atoi(argVector[i]);
	else return defaultValue;
}
//---------------------------------------------------------------------

inline CString nCmdLineArgs::GetStringArg(const CString& option, const CString& defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return argVector[i];
	else return defaultValue;
}
//---------------------------------------------------------------------

#endif
