#pragma once
#include <Data/Ptr.h>
#include <Data/RefCounted.h>
#include <string_view>

// A base class for resolving text string with embedded variables.
// Subclass this class for different variable logic.

namespace /*DEM::*/Data
{
using PTextResolver = Ptr<class ITextResolver>;

class ITextResolver : public ::Data::CRefCounted
{
public:

	virtual bool ResolveToken(std::string_view In, std::string& Out) = 0;

	std::string Resolve(std::string_view In) { std::string Result; ResolveTo(In, Result); return Result; }
	void ResolveTo(std::string_view In, std::string& Out); //???TODO: return bool? if error or if no tokens in the source?
};

class CCompositeTextResolver : public ITextResolver
{
public:

	CCompositeTextResolver(std::initializer_list<PTextResolver> SubResolvers)
		: _SubResolvers(std::move(SubResolvers))
	{}

	virtual bool ResolveToken(std::string_view In, std::string& Out) override;

protected:

	std::vector<PTextResolver> _SubResolvers;
};

}
