#pragma once
#ifndef __DEM_L1_AUDIO_H__
#define __DEM_L1_AUDIO_H__

#include <StdDEM.h>
#include <string.h>

// Audio subsystem forward declarations

namespace Audio
{

enum ESoundCategory
{
	Effect = 0,
	Music,
	Speech,
	Ambient,
	SoundCategoryCount,
	InvalidSoundCategory,
};

const char*		SoundCategoryToString(ESoundCategory Cat);
ESoundCategory	StringToSoundCategory(const char* pStr);

}

#endif

