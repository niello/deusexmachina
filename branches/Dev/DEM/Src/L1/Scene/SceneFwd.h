#pragma once
#ifndef __DEM_L1_SCENE_FWD_H__
#define __DEM_L1_SCENE_FWD_H__

// Scene graph forward declarations

namespace Scene
{

enum EChannel
{
	Chnl_Translation	= 0x01,
	Chnl_Rotation		= 0x02,
	Chnl_Scaling		= 0x04
};

}

#endif