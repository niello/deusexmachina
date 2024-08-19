#pragma once
#include <string_view>

// A base class for resolving text string with embedded variables.
// Subclass this class for different variable logic.

namespace DEM::Data
{

class ITextResolver
{
public:

	virtual bool GetVariable(std::string_view ID, std::string& Out) = 0;

	std::string Resolve(std::string_view Input);
};

}
