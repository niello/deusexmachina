#include "ParamsTextResolver.h"

using namespace std::string_view_literals;

namespace /*DEM::*/Data
{

bool CParamsTextResolver::ResolveToken(std::string_view In, CStringAppender Out)
{
	if (!_Params) return false;

	auto* pParam = _Params->Find(In.substr(1, In.size() - 2));
	if (!pParam) return false;

	// TODO: can make DEM::Meta::compile_switch or std::visit for CData?! Add CData::Visit?! Use the same visitor as std, i.e. support generic lambda.
	//!!!need to build a list of CData registered types with their TypeID indices. Use std::variant or tuple or smth like that for it? Simple integral_constant?
	/* Could adapt this recursive approach for a list of types? Or reuse above mentioned functions?
	using V = std::variant<A, B, C>;

	template <std::size_t I = 0>
	V parse(const json& j)
	{
		if constexpr (I < std::variant_size_v<V>)
		{
			auto result = j.get<std::optional<std::variant_alternative_t<I, V>>>();
			return result ? std::move(*result) : parse<I + 1>(j);
		}
		throw ParseError("Can't parse");
	}
	*/

	const auto& Value = pParam->GetRawValue();
	switch (Value.GetTypeID())
	{
		case CTypeID<bool>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<bool>())); return true;
		case CTypeID<int>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<int>())); return true;
		case CTypeID<float>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<float>())); return true;
		case CTypeID<CString>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<CString>())); return true;
		case CTypeID<CStrID>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<CStrID>())); return true;
		case CTypeID<vector3>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<vector3>())); return true;
		case CTypeID<vector4>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<vector4>())); return true;
		case CTypeID<matrix44>::TypeID: Out.Append(StringUtils::ToString(Value.GetValue<matrix44>())); return true;
		default: return false;
	}

	return false;
}
//---------------------------------------------------------------------

}
