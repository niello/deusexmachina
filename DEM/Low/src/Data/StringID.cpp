#include "StringID.h"
#include "StringIDStorage.h"

namespace Data
{
CStringIDStorage CStringID::Storage;

const CStringID CStringID::Empty;

CStringID::CStringID(std::string_view Str, bool OnlyExisting)
	: pString(Str.empty() ? CStringID::Empty.CStr() : OnlyExisting ? Storage.Get(Str).CStr() : Storage.GetOrAdd(Str).CStr())
{
}
//---------------------------------------------------------------------

}
