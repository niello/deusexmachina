#pragma once
#ifndef __DEM_L1_DB_STD_ATTRS_H__
#define __DEM_L1_DB_STD_ATTRS_H__

#include "AttrID.h"

// Standard and system DB attributes

namespace Attr
{
	DeclareInt(rowid);
	DeclareInt(ID);
	DeclareStrID(GUID);
}

#endif
