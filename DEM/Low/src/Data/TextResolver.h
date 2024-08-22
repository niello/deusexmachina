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
protected:

	std::vector<PTextResolver> _SubResolvers;

public:

	CCompositeTextResolver(std::initializer_list<PTextResolver> SubResolvers)
		: _SubResolvers(std::move(SubResolvers))
	{}

	virtual bool ResolveToken(std::string_view In, std::string& Out) override;
};

class CMapTextResolver : public ITextResolver
{
public:

	// TODO: can make improved versions with trie or with perfect hashing, e.g. for localization
	// TODO: C++20 transparent comparator
	std::unordered_map<std::string, std::string> _Map;

	CMapTextResolver() = default;
	CMapTextResolver(std::initializer_list<decltype(_Map)::value_type> Data)
		: _Map(std::move(Data))
	{}

	virtual bool ResolveToken(std::string_view In, std::string& Out) override
	{
		auto It = _Map.find(std::string(In.substr(1, In.size() - 2)));
		if (It == _Map.cend()) return false;
		Out = It->second;
		return true;
	}
};

}
