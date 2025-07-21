#pragma once
#include <Data/RefCounted.h>
#include <Data/Data.h>

// Array of variant variables

namespace Data
{

class CDataArray: public std::vector<CData>, public Data::CRefCounted
{
public:

	CData&			Get(size_t Index) { return at(Index); }
	const CData&	Get(size_t Index) const { return at(Index); }
	template<class T>
	T&				Get(size_t Index) { return at(Index).GetValue<T>(); }
	template<class T>
	const T&		Get(size_t Index) const { return at(Index).GetValue<T>(); }
};
//---------------------------------------------------------------------

// Crucial for Sol automagic enrollments which try instantiating operators for any vector
bool operator <(const CDataArray&, const CDataArray&) = delete;
bool operator <=(const CDataArray&, const CDataArray&) = delete;
bool operator >(const CDataArray&, const CDataArray&) = delete;
bool operator >=(const CDataArray&, const CDataArray&) = delete;

using PDataArray = Ptr<CDataArray>;

}

DECLARE_TYPE(PDataArray, 10)
