#include "StringUtils.h"

namespace StringUtils
{

// A string matching function using Tcl's matching rules.
bool MatchesPattern(const char* pStr, const char* pPattern)
{
	char c2;

	while (true)
	{
		if (!*pPattern) return !*pStr;
		if (!*pStr && *pPattern != '*') return false;
		if (*pPattern == '*')
		{
			++pPattern;
			if (!*pPattern) return true;
			while (true)
			{
				if (MatchesPattern(pStr, pPattern)) return true;
				if (!*pStr) return false;
				++pStr;
			}
		}
		if (*pPattern == '?') goto match;
		if (*pPattern == '[')
		{
			++pPattern;
			while (true)
			{
				if (*pPattern == ']' || !*pPattern) return false;
				if (*pPattern == *pStr) break;
				if (pPattern[1] == '-')
				{
					c2 = pPattern[2];
					if (!c2) return false;
					if (*pPattern <= *pStr && c2 >= *pStr) break;
					if (*pPattern >= *pStr && c2 <= *pStr) break;
					pPattern += 2;
				}
				++pPattern;
			}
			while (*pPattern != ']')
			{
				if (!*pPattern)
				{
					--pPattern;
					break;
				}
				++pPattern;
			}
			goto match;
		}

		if (*pPattern == '\\')
		{
			++pPattern;
			if (!*pPattern) return false;
		}
		if (*pPattern != *pStr) return false;

match:
		++pPattern;
		++pStr;
	}
}
//---------------------------------------------------------------------

CString FromMatrix44(const matrix44& m)
{
	CString Str;
	Str.Format(	"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f, "
				"%.6f, %.6f, %.6f, %.6f",
				m.M11, m.M12, m.M13, m.M14,
				m.M21, m.M22, m.M23, m.M24,
				m.M31, m.M32, m.M33, m.M34,
				m.M41, m.M42, m.M43, m.M44);
	return Str;
}
//---------------------------------------------------------------------

bool ToBool(const char* pStr)
{
	static const char* Bools[] = { "no", "yes", "off", "on", "false", "true", NULL };
	int i = 0;
	while (Bools[i])
	{
		if (!n_stricmp(Bools[i], pStr)) return (i & 1);
		++i;
	}
	Sys::Error("Invalid string value for bool!\n");
	FAIL;
}
//---------------------------------------------------------------------

}
