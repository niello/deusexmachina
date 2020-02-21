#pragma once
#include <IO/Stream.h>
#include <Data/Ptr.h>

// Serializer for HRD files

//???to generic TextWriter?
// HRD is just a text serialization for CParams/CParam/CData

namespace Data
{
	class CParam;
	class CData;
	typedef Ptr<class CParams> PParams;
}

namespace IO
{

class CHRDWriter final
{
protected:

	IStream& Stream;

	int CurrTabLevel;
	//!!!CFlags Flags; like OneLineArray, Separator(TabChar) etc!
	
	bool WriteIndent();

public:

	CHRDWriter(IStream& DestStream) : Stream(DestStream) {}

	bool WriteParams(const Data::CParams& Value);
	bool WriteParam(const Data::CParam& Value);
	bool WriteData(const Data::CData& Value);

	inline bool WriteCharString(const char* pString)
	{
		const UPTR Len = pString ? strlen(pString) : 0;
		return Stream.Write(pString, Len) == Len;
	}

	IStream& GetStream() const { return Stream; }
};

}
