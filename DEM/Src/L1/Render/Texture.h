#pragma once
#ifndef __DEM_L1_RENDER_TEXTURE_H__
#define __DEM_L1_RENDER_TEXTURE_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>

// Texture resource object that may be stored in VRAM and used for rendering

namespace Render
{

class CTexture: public Resources::CResourceObject
{
public:

	enum EType
	{
		InvalidType,
		Texture2D,
		Texture3D,
		TextureCube
	};

	enum ECubeFace
	{
		PosX = 0,
		NegX,
		PosY,
		NegY,
		PosZ,
		NegZ
	};

	struct CMapInfo
	{        
		void*	pData;
		int		RowPitch;
		int		DepthPitch;

		CMapInfo(): pData(NULL), RowPitch(0), DepthPitch(0) {}
	};

protected:

	//???desc structure?
	//EType					Type;
	//DWORD					Width;
	//DWORD					Height;
	//DWORD					Depth;
	//DWORD					MipCount;
	//EPixelFormat			PixelFormat;

	//DWORD					LockCount;

public:

	//Data::CFlags	Access; //!!!can use as generic flags!

	//CTexture();
	//virtual ~CTexture() { if (IsLoaded()) Unload(); }
};

typedef Ptr<CTexture> PTexture;

}

//DECLARE_TYPE(Render::PTexture, 14)
//#define TTexture DATA_TYPE(Render::PTexture)

#endif
