#include "TextResolver.h"
#include <System/System.h>
//#include <stack>

namespace DEM::Data
{

std::string ITextResolver::Resolve(std::string_view Input)
{
	const size_t InputLength = Input.size();

	//!!!TODO: more sophisticated estimation? count { and } braces on the top level and for each add max(id_len, max_expected_var_len)?
	std::string Result;
	Result.reserve(InputLength);

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
			{
				Result += '{';
				i += 2;
			}
			else
			{
				BracePosStack.push_back(Result.size());
				Result += '{';
				++i;
			}
		}
		else if (Chr == '}')
		{
			if (i + 1 < InputLength && Input[i + 1] == '}')
			{
				Result += '}';
				i += 2;
			}
			else if (!BracePosStack.empty()) //!!!need to fire error if empty?!
			{
				const size_t TokenStartPos = BracePosStack.back();
				BracePosStack.pop_back();
				std::string variable_id = Result.substr(TokenStartPos + 1, Result.size() - TokenStartPos - 2);

				// Attempt to resolve the variable
				//!!!can pass result inside and use something like fmt::format_to(back_inserter), to avoid temporary var!
				//!!!all resolvers then must write results to output string, not construct new one to return! And also can use retval for bool success then!
				if (GetVariable(variable_id, Result))
				{
					// Replace the part of the result buffer starting from `{` with its resolved value
					//!!!must happen inside? send position from which to start inserting? how to optimize?
					//???can live without tmp string for var resolve result?
					//Result.erase(TokenStartPos);
					//Result += variable_value;

					//!!!need to restart from substitution point, because substituted value can have tokens!!!
					//!!!and must iterate result, not input!!! first copy one to another?! or use insertion in the middle? slow!!!
					++i;
				}
				else
				{
					// Unresolved variable tokens are left in the text as is, so append `}` to complete it
					Result += '}';
					++i;
				}
			}
		}
		else
		{
			Result += Chr;
			++i;
		}
	}

	n_assert(Stack.empty());

	return Result;
}
//---------------------------------------------------------------------

}
