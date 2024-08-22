#include "TextResolver.h"
#include <System/System.h>

namespace /*DEM::*/Data
{

//???TODO: instead of return value pass Output as arg? Can then reuse preallocated buffer in recursion and control preallocation better overall.
//???how to avoid resolving text without '{'? Maybe pass input as In-Out string / buffer?
std::string ITextResolver::Resolve(std::string_view Input)
{
	const size_t InputLength = Input.size();

	//!!!TODO: more sophisticated estimation? count { and } braces on the top level and for each add max(id_len, max_expected_var_len)?
	std::string Result;
	Result.reserve(Result.size() + InputLength);

	//!!!TODO: limit nesting depth with e.g. 16 and use std::array + cursor?!
	std::vector<size_t> BracePosStack;
	BracePosStack.reserve(8);

	size_t i = 0;
	while (i < InputLength)
	{
		const char Chr = Input[i];
		if (Chr == '{')
		{
			if (i + 1 < InputLength && Input[i + 1] == '{')
				++i;
			else
				BracePosStack.push_back(Result.size());

			// Write anyway because unresolved tokens are kept in result as is
			Result += '{';
		}
		else if (Chr == '}')
		{
			// Token is sent to resolver with surrounding braces
			Result += '}';

			if (i + 1 < InputLength && Input[i + 1] == '}')
			{
				++i;
			}
			else if (!BracePosStack.empty())
			{
				const size_t TokenStartPos = BracePosStack.back();
				BracePosStack.pop_back();
				const auto Token = std::string_view{ Result }.substr(TokenStartPos);

				// Attempt to resolve the token
				// NB: resolvers should end reading Token before starting writing to Result!
				if (ResolveToken(Token, Result))
				{
					// Resolve tokens and escaped braces in a text resolved from the token
					const auto ResolvedText = std::string_view{ Result }.substr(TokenStartPos);
					if (ResolvedText.find('{') != std::string_view::npos || ResolvedText.find('}') != std::string_view::npos)
					{
						//!!!can resolve from found '{' or '}'! preceding text is verbatim!
						const auto SubResult = Resolve(ResolvedText);
						Result.erase(TokenStartPos);
						Result += SubResult;
					}
				}
			}
			else
			{
				::Sys::Error("ITextResolver::Resolve() > closing brace '}' without matching opening brace '{'");
			}
		}
		else
		{
			// Regular character, just add it to result
			Result += Chr;
		}

		++i;
	}

	n_assert2(BracePosStack.empty(), "ITextResolver::Resolve() > opening brace '{' without matching closing brace '}'");

	return Result;
}
//---------------------------------------------------------------------

bool CCompositeTextResolver::ResolveToken(std::string_view In, std::string& Out)
{
	for (auto& SubResolver : _SubResolvers)
		if (ResolveToken(In, Out)) return true;
	return false;
}
//---------------------------------------------------------------------

}
