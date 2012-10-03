#pragma once
#ifndef __DEM_L1_HRD_WRITER_H__
#define __DEM_L1_HRD_WRITER_H__

#include <Data/StreamWriter.h>
#include <Core/Ptr.h>

// Serializer for HRD files

//???to generic TextWriter?
// HRD is just a text serialization for CParams/CParam/CData

namespace Data
{
class CParam;
class CData;
typedef Ptr<class CParams> PParams;

class CHRDWriter: public CStreamWriter
{
protected:

	int CurrTabLevel;
	//!!!CFlags Flags; like OneLineArray, Separator(TabChar) etc!
	
	bool WriteIndent();

public:

	CHRDWriter(CStream& DestStream): CStreamWriter(DestStream) { }

	bool WriteParams(PParams Value);
	bool WriteParam(const CParam& Value);
	bool WriteData(const CData& Value);
};

}

#endif
