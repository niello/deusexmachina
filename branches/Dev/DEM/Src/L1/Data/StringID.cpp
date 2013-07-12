#include "StringID.h"
#include "StringIDStorage.h"

namespace Data
{
static CStringIDStorage StringIDStorage;

//CStringIDStorage* CStringID::Storage = n_new(CStringIDStorage()); //???!!!where to delete and is it needed?
CStringIDStorage* CStringID::Storage = &StringIDStorage;

const CStringID CStringID::Empty;

CStringID::CStringID(LPCSTR pString, bool OnlyExisting)
{
	if (pString && *pString)
		String = OnlyExisting ? Storage->GetIDByString(pString) : Storage->AddString(pString);
	else String = CStringID::Empty.CStr();
}
//---------------------------------------------------------------------

}