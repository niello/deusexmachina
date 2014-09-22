#pragma once
#ifndef __DEM_L1_SCENE_FWD_H__
#define __DEM_L1_SCENE_FWD_H__

// Scene graph forward declarations

namespace Scene
{

enum ETransformChannel
{
	Tfm_Translation	= 0x01,
	Tfm_Rotation	= 0x02,
	Tfm_Scaling		= 0x04
};

}

#endif