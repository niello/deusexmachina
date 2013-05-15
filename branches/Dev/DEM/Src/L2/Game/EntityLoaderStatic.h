#pragma once
#ifndef __DEM_L2_ENTITY_LOADER_STATIC_H__
#define __DEM_L2_ENTITY_LOADER_STATIC_H__

#include "EntityLoader.h"

// This loader tries to load an entity as a static geometry (graphics and collision only) and
// doesn't create an entity object

namespace Game
{

class CEntityLoaderStatic: public IEntityLoader
{
	__DeclareClass(CEntityLoaderStatic);

public:

	virtual bool Load(CStrID UID, CStrID LevelID, Data::PParams Desc);
};

__RegisterClassInFactory(CEntityLoaderStatic);

}

#endif
