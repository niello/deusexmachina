#include "StringID.h"
#include "StringIDStorage.h"

namespace Data
{
const CStringID CStringID::Empty;

CStringIDStorage& GetStorage()
{
	static CStringIDStorage Storage;
	return Storage;
}
//---------------------------------------------------------------------

CStringID CStringID::Find(std::string_view Str)
{
	return GetStorage().Get(Str);
}
//---------------------------------------------------------------------

CStringID::CStringID(std::string_view Str, bool OnlyExisting)
	: pString(Str.empty() ? CStringID::Empty.CStr() : OnlyExisting ? GetStorage().Get(Str).CStr() : GetStorage().GetOrAdd(Str).CStr())
{
}
//---------------------------------------------------------------------

}
