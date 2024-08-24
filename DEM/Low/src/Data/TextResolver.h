#pragma once
#include <Data/Ptr.h>
#include <Data/RefCounted.h>
#include <Data/StringUtils.h>

// A base class for resolving text string with embedded variables.
// Subclass ITextResolver for different variable logic.
// Some common implementations are included in this header.

namespace /*DEM::*/Data
{
using PTextResolver = Ptr<class ITextResolver>;
using CStringAppender = StringUtils::CStringAppender;

class ITextResolver : public ::Data::CRefCounted
{
public:

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) = 0;

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

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) override;
};

inline PTextResolver CreateTextResolver(std::initializer_list<PTextResolver> SubResolvers)
{
	return new CCompositeTextResolver(std::move(SubResolvers));
}
//---------------------------------------------------------------------

class CMapTextResolver : public ITextResolver
{
public:

	// TODO: can make improved versions with trie or with perfect hashing, e.g. for localization
	// TODO: C++20 transparent comparator
	std::unordered_map<std::string, std::string> _Map;

	CMapTextResolver() = default;
	CMapTextResolver(std::initializer_list<std::pair<const std::string, std::string>> Data)
		: _Map(std::move(Data))
	{}

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) override
	{
		auto It = _Map.find(std::string(In.substr(1, In.size() - 2)));
		if (It == _Map.cend()) return false;
		Out.Append(It->second);
		return true;
	}
};

inline PTextResolver CreateTextResolver(std::initializer_list<std::pair<const std::string, std::string>> Data)
{
	return new CMapTextResolver(std::move(Data));
}
//---------------------------------------------------------------------

}
