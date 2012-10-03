#pragma once
#ifndef __DEM_L2_CHARACTER_PROFILE_H__
#define __DEM_L2_CHARACTER_PROFILE_H__

#include "UserProfile.h"

// A character profile stores general data about the player/character.
// Character profiles are stored in "user:[appname]/character/[profilename]".
// Based on mangalore CharacterProfile_(C) 2006 Radon Labs GmbH

namespace Profiles
{

class CharacterProfile: public UserProfile
{
	DeclareRTTI;
	DeclareFactory(CharacterProfile);

public:

	virtual ~CharacterProfile();

	virtual bool Open(const nString& PrfName);
};

RegisterFactory(CharacterProfile);

}

#endif