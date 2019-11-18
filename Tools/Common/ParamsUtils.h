#pragma once
#include <DataScheme.h>
#include <algorithm>
#include <iostream>

// Utilities for CParams and descs (description format based on HRD/PRM with inheritance support)

namespace ParamsUtils
{

bool LoadParamsFromHRD(const char* pFileName, Data::CParams& OutParams);
bool LoadParamsFromPRM(const char* pFileName, Data::CParams& OutParams);
bool LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName, Data::CParams& OutParams);
bool LoadSchemes(const char* pFileName, Data::CSchemeSet& OutSchemes);
bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params);
bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params);
bool SaveParamsByScheme(std::ostream& Stream, const Data::CParams& Params, CStrID SchemeID, const Data::CSchemeSet& SchemeSet);
bool SaveParamsByScheme(const char* pFileName, const Data::CParams& Params, CStrID SchemeID, const Data::CSchemeSet& SchemeSet);

inline Data::CParamsSorted OrderedParamsToSorted(const Data::CParams& ParamsOrdered)
{
	return Data::CParamsSorted(ParamsOrdered.cbegin(), ParamsOrdered.cend());
}
//---------------------------------------------------------------------

inline bool HasParam(const Data::CParams& Params, CStrID Key)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key](const auto& Param) { return Param.first == Key; });
	return It != Params.cend();
}
//---------------------------------------------------------------------

inline bool HasParam(const Data::CParamsSorted& Params, CStrID Key)
{
	return Params.find(Key) != Params.cend();
}
//---------------------------------------------------------------------

template<class T>
const T& GetParam(const Data::CParams& Params, const char* pKey, const T& Default)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	return (It == Params.cend()) ? Default : It->second;
}
//---------------------------------------------------------------------

template<class T>
const bool TryGetParam(T& Out, Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend() || !It->second.IsA<T>()) return false;
	Out = It->second.GetValue<T>();
	return true;
}
//---------------------------------------------------------------------

template<class T>
const bool TryGetParam(T& Out, const Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend() || !It->second.IsA<T>()) return false;
	Out = It->second.GetValue<T>();
	return true;
}
//---------------------------------------------------------------------

template<class T>
const T& GetParam(const Data::CParamsSorted& Params, const char* pKey, const T& Default)
{
	auto It = Params.find(CStrID(pKey));
	return (It == Params.cend()) ? Default : It->second;
}
//---------------------------------------------------------------------

template<class T>
const bool TryGetParam(T& Out, Data::CParamsSorted& Params, const char* pKey)
{
	auto It = Params.find(CStrID(pKey));
	if (It == Params.cend() || !It->second.IsA<T>()) return false;
	Out = It->second.GetValue<T>();
	return true;
}
//---------------------------------------------------------------------

// FIXME: hell
template<> const bool TryGetParam(Data::CData& Out, const Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CData*& Out, const Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CParams*& Out, const Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CDataArray*& Out, const Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CData*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CParams*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(const Data::CDataArray*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(Data::CData*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(Data::CParams*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(Data::CDataArray*& Out, Data::CParams& Params, const char* pKey);
template<> const bool TryGetParam(Data::CData& Out, Data::CParamsSorted& Params, const char* pKey);

}
