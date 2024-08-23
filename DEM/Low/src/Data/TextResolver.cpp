#include "TextResolver.h"
#include <System/System.h>

using namespace std::string_view_literals;

namespace /*DEM::*/Data
{

void ITextResolver::ResolveTo(std::string_view In, std::string& Out)
{
	const size_t InputLength = In.size();

	//!!!TODO: more sophisticated estimation? count { and } braces on the top level and for each add max(id_len, max_expected_var_len)?
	//could also remember pos of the first '{' then.
	Out.reserve(Out.size() + InputLength);

	constexpr size_t NestingLimit = 16;
	std::array<size_t, NestingLimit> BracePosStack;
	size_t CurrNesting = 0;

	// Reusable buffer for possible recursive resolutions
	std::string SubResult;

	size_t i = 0;
	while (i < InputLength)
	{
		const char Chr = In[i];
		if (Chr == '{')
		{
			if (CurrNesting < NestingLimit)
				BracePosStack[CurrNesting] = Out.size();
			else
				::Sys::Error("ITextResolver::Resolve() > token nesting limit exceeded");

			++CurrNesting;

			// Write because unresolved tokens are kept in result as is
			Out += '{';
		}
		else if (Chr == '}')
		{
			// Token is sent to resolver with surrounding braces
			Out += '}';

			if (CurrNesting > NestingLimit)
			{
				// Keep counting without resolving too deeply nested tokens
				--CurrNesting;
			}
			else if (CurrNesting)
			{
				const size_t TokenStartPos = BracePosStack[--CurrNesting];
				const auto Token = std::string_view{ Out }.substr(TokenStartPos);

				// Attempt to resolve the token
				// NB: resolvers should end reading Token before starting writing to Out because they point to the same location!
				if (ResolveToken(Token, { Out, TokenStartPos }))
				{
					// Recursively resolve tokens and escaped braces in a text resolved from the token
					const auto ResolvedText = std::string_view{ Out }.substr(TokenStartPos);
					const size_t SpecialChrPos = ResolvedText.find_first_of("\\{}"sv);
					if (SpecialChrPos != std::string_view::npos)
					{
						ResolveTo(ResolvedText.substr(SpecialChrPos), SubResult);
						Out.erase(TokenStartPos + SpecialChrPos);
						Out += SubResult;
						SubResult.clear();
					}
				}
			}
			else
			{
				::Sys::Error("ITextResolver::Resolve() > closing brace '}' without matching opening brace '{'");
			}
		}
		else if (Chr == '\\' && (i + 1 < InputLength))
		{
			// Process escape sequences \\, \{ and \}
			const char NextChar = In[i + 1];
			if (NextChar == '\\' || NextChar == '{' || NextChar == '}')
			{
				++i;
				Out += NextChar;
			}
			else
			{
				Out += Chr;
			}
		}
		else
		{
			// Regular character, just add it to result
			Out += Chr;
		}

		++i;
	}

	n_assert2(!CurrNesting, "ITextResolver::Resolve() > opening brace '{' without matching closing brace '}'");
}
//---------------------------------------------------------------------

bool CCompositeTextResolver::ResolveToken(std::string_view In, CStringAppender Out)
{
	for (auto& SubResolver : _SubResolvers)
		if (SubResolver->ResolveToken(In, Out)) return true;
	return false;
}
//---------------------------------------------------------------------

}
