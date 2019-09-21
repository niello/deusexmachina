#pragma once
#include <DataScheme.h>

// Utilities for CParams and descs (description format based on HRD/PRM with inheritance support)

namespace ParamsUtils
{

bool LoadParamsFromHRD(const char* pFileName, Data::CParams& OutParams);
bool LoadParamsFromPRM(const char* pFileName, Data::CParams& OutParams);
bool LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName, Data::CParams& OutParams);
bool LoadDataSerializationSchemesFromDSS(const char* pFileName, std::map<CStrID, Data::CDataScheme>& OutSchemes);
bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params);
bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params);

}
