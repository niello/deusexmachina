#pragma once
#ifndef __DEM_L2_ENV_LOADER_H__
#define __DEM_L2_ENV_LOADER_H__

#include "EntityLoaderBase.h"

// Helper class which loads all the environment objects into a level.
// Based on mangalore EnvironmentLoader_(C) 2006 RadonLabs GmbH

namespace Loading
{
class CEnvironmentLoader: public CEntityLoaderBase
{
	DeclareRTTI;
	DeclareFactory(CEnvironmentLoader);

private:

	bool HasCollide(const nString& RsrcName);
	bool HasPhysics(const nString& RsrcName);

public:

	virtual bool Load(CStrID Category, DB::CValueTable* pTable, int RowIdx);
};

RegisterFactory(CEnvironmentLoader);

}

#endif
