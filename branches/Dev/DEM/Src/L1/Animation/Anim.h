#pragma once
#ifndef __DEM_L1_ANIM_H__
#define __DEM_L1_ANIM_H__

// Animation system constants and forward declarations

namespace Anim
{

enum EChannel
{
	Chnl_Translation	= 0x01,
	Chnl_Rotation		= 0x02,
	Chnl_Scaling		= 0x04
};

/*
enum ELoopType
{
	LoopType_Clamp,
	LoopType_Loop
};
*/

}

#endif