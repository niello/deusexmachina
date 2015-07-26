#include "StringID.h"
#include "StringIDStorage.h"

namespace Data
{
CStringIDStorage CStringID::Storage;

const CStringID CStringID::Empty;

CStringID::CStringID(LPCSTR pString, bool OnlyExisting)
{
	if (pString && *pString)
		String = OnlyExisting ? Storage.Get(pString) : Storage.GetOrAdd(pString);
	else String = CStringID::Empty.CStr();
}
//---------------------------------------------------------------------

}