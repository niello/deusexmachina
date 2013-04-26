#include "StringID.h"
#include "StrIDStorage.h"

namespace Data //???need?
{
static CStrIDStorage StrIDStorage;

//CStrIDStorage* CStringID::Storage = n_new(CStrIDStorage()); //???!!!where to delete and is it needed?
CStrIDStorage* CStringID::Storage = &StrIDStorage;

const CStringID CStringID::Empty;

CStringID::CStringID(LPCSTR string, bool OnlyExisting)
{
	if (string && string[0])
	{
		if (OnlyExisting) String = Storage->GetIDByString(string);
		else String = Storage->AddString(string);
	}
	else String = NULL; //Storage.GetEmptyStrID();
}
//---------------------------------------------------------------------

}