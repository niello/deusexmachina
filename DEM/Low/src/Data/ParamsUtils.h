#pragma once
#ifndef __DEM_L1_PARAMS_UTILS_H__
#define __DEM_L1_PARAMS_UTILS_H__

#include <Data/Ptr.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

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
bool LoadDataSerializationSchemesFromDSS(const char* pFileName, CDict<CStrID, Data::PDataScheme>& OutSchemes);
bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params);
bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params);

}

#endif
