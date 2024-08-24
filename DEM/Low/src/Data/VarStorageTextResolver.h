#pragma once
#include <Data/TextResolver.h>
#include <Data/VarStorage.h>

// A text resolver which substitutes values from CVarStorage
// TODO: can add version with own Vars instance!

namespace /*DEM::*/Data
{

template<typename... TVarTypes>
class CVarStorageRefTextResolver : public ITextResolver
{
protected:

	const CVarStorage<TVarTypes...>& _Vars;

public:

	CVarStorageRefTextResolver(const CVarStorage<TVarTypes...>& Vars) : _Vars(Vars) {}

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) override
	{
		//!!!DBG TMP! Need to fix CStrID expecting a null terminated string!!!
		//if (auto Handle = _Vars.Find(In.substr(1, In.size() - 2)))
		if (auto Handle = _Vars.Find(std::string(In.substr(1, In.size() - 2))))
		{
			//!!!TODO: need to implement CVarStorage::Visit with return value, like std::visit!
			_Vars.Visit(Handle, [&Out](auto&& SubValue) { Out.Append(StringUtils::ToString(SubValue)); });
			return true;
		}

		return false;
	}
};

template<typename... TVarTypes>
PTextResolver CreateTextResolver(const CVarStorage<TVarTypes...>& Vars)
{
	return new CVarStorageRefTextResolver(Vars);
}
//---------------------------------------------------------------------

}
