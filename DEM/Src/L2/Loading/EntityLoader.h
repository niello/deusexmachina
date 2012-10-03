#pragma once
#ifndef __DEM_L2_ENTITY_LOADER_H__
#define __DEM_L2_ENTITY_LOADER_H__

#include "EntityLoaderBase.h"

// Loader helper for universal game entities. The properties which are
// attached to the entity are described in blueprints.xml, the attributes
// to attach come from the world database.
// Based on mangalore EntityLoader_(C) 2006 RadonLabs GmbH

namespace Loading
{

class CEntityLoader: public CEntityLoaderBase
{
	DeclareRTTI;
	DeclareFactory(CEntityLoader);

public:

	virtual bool Load(CStrID Category, DB::CValueTable* pTable, int RowIdx);
};

RegisterFactory(CEntityLoader);

}

#endif