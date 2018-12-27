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

	int FindArg(const char* option) const;

public:

	nCmdLineArgs(): argCount(0), argVector(NULL) {}
	nCmdLineArgs(int argc, const char** argv): argCount(argc), argVector(argv) {}

	void		Initialize(int argc, const char** argv) { argCount = argc; argVector = argv; }
	float		GetFloatArg(const char* option, float defaultValue = 0.0f) const;
	int			GetIntArg(const char* option, int defaultValue = 0) const;
	bool		GetBoolArg(const char* option) const { return FindArg(option) > 0; }
	const char*	GetStringArg(const char* option, const char* defaultValue = NULL) const;
	bool		HasArg(const CString& option) const { return FindArg(option) != 0; }
};

inline int nCmdLineArgs::FindArg(const char* option) const
{
	for (int i = 0; i < argCount; i++)
		if (!strcmp(option, argVector[i]))
			return i;
	return -1;
}
//---------------------------------------------------------------------

inline float nCmdLineArgs::GetFloatArg(const char* option, float defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return (float)atof(argVector[i]);
	else return defaultValue;
}
//---------------------------------------------------------------------

inline int nCmdLineArgs::GetIntArg(const char* option, int defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return atoi(argVector[i]);
	else return defaultValue;
}
//---------------------------------------------------------------------

inline const char* nCmdLineArgs::GetStringArg(const char* option, const char* defaultValue) const
{
	int i = FindArg(option);
	if (i == -1) return defaultValue;
	else if (++i < argCount) return argVector[i];
	else return defaultValue;
}
//---------------------------------------------------------------------

#endif
