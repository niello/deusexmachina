#pragma once
#include <iostream>
#include <vector>

struct CGlobalTable
{
	struct CSource
	{
		std::shared_ptr<std::istream> Stream;
		uint32_t Offset;
	};

	std::vector<CSource> Sources;
	std::string Result;
};
