#include "StdAttrs.h"

#include <DB/DBServer.h>

namespace Attr
{
	DefineInt(rowid);
	DefineInt(ID);
	DefineStrID(GUID);
};

BEGIN_ATTRS_REGISTRATION(StdAttrs)
	RegisterIntWithDefault(rowid, ReadOnly, -1); //???readwrite?
	RegisterIntWithDefault(ID, ReadOnly, 0); //???readwrite?
	RegisterStrID(GUID, ReadOnly);
END_ATTRS_REGISTRATION
