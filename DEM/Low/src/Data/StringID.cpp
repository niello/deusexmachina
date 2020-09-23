#include "StringID.h"
#include "StringIDStorage.h"

namespace Data
{
CStringIDStorage CStringID::Storage;

const CStringID CStringID::Empty;

CStringID::CStringID(const char* pStr, bool OnlyExisting)
{
	if (pStr && *pStr)
		pString = OnlyExisting ? Storage.Get(pStr).CStr() : Storage.GetOrAdd(pStr).CStr();
	else pString = CStringID::Empty.CStr();
}
//---------------------------------------------------------------------

}
