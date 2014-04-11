#pragma once
#ifndef __DEM_L2_ENTITY_LOADER_COMMON_H__
#define __DEM_L2_ENTITY_LOADER_COMMON_H__

#include "EntityLoader.h"

// Common entity loader always loads a new entity instance

namespace Game
{

class CEntityLoaderCommon: public IEntityLoader
{
	__DeclareClass(CEntityLoaderCommon);

public:

	virtual bool Load(CStrID UID, CGameLevel& Level, const Data::CParams& Desc);
};

}

#endif