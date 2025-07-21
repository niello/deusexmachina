#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <map>

// Utilities for CParams and descs (description format based on HRD/PRM with inheritance support)

namespace Data
{
	typedef Ptr<class CParams> PParams;
	typedef Ptr<class CDataScheme> PDataScheme;
}

namespace ParamsUtils
{

Data::PParams LoadParamsFromHRD(const char* pFileName);
Data::PParams LoadParamsFromPRM(const char* pFileName);
Data::PParams LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName);
bool LoadDataSerializationSchemesFromDSS(const char* pFileName, std::map<CStrID, Data::PDataScheme>& OutSchemes);
bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params);
bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params);

}

namespace StringUtils
{

std::string ToString(const Data::CParams& Value);

}
