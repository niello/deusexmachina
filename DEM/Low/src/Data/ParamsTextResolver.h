#pragma once
#include <Data/TextResolver.h>
#include <Data/Params.h>

// A text resolver which substitutes values from CParams

namespace /*DEM::*/Data
{

class CParamsTextResolver : public ITextResolver
{
public:

	PParams _Params;

	CParamsTextResolver() = default;
	CParamsTextResolver(PParams Params) : _Params(std::move(Params)) {}

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) override;
};

inline PTextResolver CreateTextResolver(PParams Params)
{
	return new CParamsTextResolver(std::move(Params));
}
//---------------------------------------------------------------------

}
