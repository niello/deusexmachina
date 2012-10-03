#pragma once
#ifndef __DEM_L2_PROP_ABSTRACT_GRAPHICS_H__
#define __DEM_L2_PROP_ABSTRACT_GRAPHICS_H__

#include <Game/Property.h>

//???need?

// Base class for all Graphics Properties.
// Based on mangalore AbstractGraphicsProperty (C) 2006 Radon Labs GmbH

namespace Properties
{

class CPropAbstractGraphics: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropAbstractGraphics);
	DeclarePropertyPools(Game::LivePool);

public:

	//???virtual visibility handler here?

	virtual nString GetGraphicsResource();
};

RegisterFactory(CPropAbstractGraphics);

}

#endif
