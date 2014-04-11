#pragma once
#ifndef __DEM_L2_ENTITY_LOADER_H__
#define __DEM_L2_ENTITY_LOADER_H__

#include <Core/RefCounted.h>
#include <Data/Params.h>

// Abstract loader helper for game entities.
// Based on mangalore EntityLoaderBase_(C) 2006 RadonLabs GmbH

namespace Game
{
class CGameLevel;

class IEntityLoader: public Core::CRefCounted
{
	__DeclareClassNoFactory;

public:

	virtual bool Load(CStrID UID, CGameLevel& Level, const Data::CParams& Desc) = 0;
};

typedef Ptr<IEntityLoader> PEntityLoader;

}

#endif