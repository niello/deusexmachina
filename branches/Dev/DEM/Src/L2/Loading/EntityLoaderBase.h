#pragma once
#ifndef __DEM_L2_ENTITY_LOADER_BASE_H__
#define __DEM_L2_ENTITY_LOADER_BASE_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Abstract loader helper for game entities.
// Based on mangalore EntityLoaderBase_(C) 2006 RadonLabs GmbH

namespace DB
{
	typedef Ptr<class CValueTable> PValueTable;
}

namespace Loading
{

class CEntityLoaderBase: public Core::CRefCounted
{
	DeclareRTTI;

public:

	virtual bool Load(CStrID Category, DB::CValueTable* pTable, int RowIdx) = 0;
};

}

#endif