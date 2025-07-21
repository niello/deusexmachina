#pragma once
#include <Data/Params.h>
#include <Data/Flags.h>
#include <Data/FourCC.h>

// Data serialization scheme declares structure of CParams list and allows to omit saving
// redundant info (types, keys etc). Use it to save CParams/HRD to custom binary files
// without any size overhead and any specific saving logic. That binaries could be read
// by specific loaders.

namespace Data
{
typedef Ptr<class CDataScheme> PDataScheme;

class CDataScheme: public Data::CRefCounted
{
public:

	enum
	{
		WRITE_KEY				= 0x01,	// Write key of this param as string (if no FourCC)
		WRITE_COUNT				= 0x02,	// Write param count of this param, if it is {} or []
		WRITE_CHILD_KEYS		= 0x04,	// Write keys of child elements, when save without subscheme or apply it to children
		WRITE_CHILD_COUNT		= 0x08,	// Write param count of child {} and [] elements, when save without subscheme or apply it to children
		APPLY_SCHEME_TO_SELF	= 0x10	// Apply scheme defined by Scheme/SchemeID to self, not to child params
	};

	struct CRecord
	{
		CStrID			ID;
		CFourCC			FourCC;
		int				TypeID; //???or type ptr?
		CStrID			SchemeID;
		PDataScheme		Scheme;
		CFlags			Flags;
		CData			Default;
	};

	std::vector<CRecord> Records;

	bool Init(const CParams& Desc);
};

}
