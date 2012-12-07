#pragma once
#ifndef __DEM_L1_GFX_TEXTURE_H__
#define __DEM_L1_GFX_TEXTURE_H__

#include <Resources/Resource.h>

#define D3D9_ONLY

#ifdef D3D9_ONLY
#include <Render/D3D9Fwd.h>
#else
#include <Gfx/PixelFormat.h>
#endif

// Texture resource, usable by renderer

//!!!OnDeviceLost, OnDeviceReset!

namespace Render
{

class CTexture: public Resources::CResource
{
	DeclareRTTI;

public:

	//!!!GfxResource flags!
	enum EUsage
	{
		UsageImmutable,      //> can only be read by GPU, not written, cannot be accessed by CPU
		UsageDynamic,        //> can only be read by GPU, can only be written by CPU
		UsageCpu,            //> a resource which is only accessible by the CPU and can't be used for rendering
	};

	//!!!GfxResource flags!
	enum EAccessMode
	{
		AccessNone,         // CPU does not require access to the resource (best)
		AccessWrite,        // CPU has write access
		AccessRead,         // CPU has read access
		AccessReadWrite,    // CPU has read/write access
	};

	enum EMapType
	{
		MapRead,                // gain read access, must be UsageDynamic and AccessRead
		MapWrite,               // gain write access, must be UsageDynamic and AccessWrite
		MapReadWrite,           // gain read/write access, must be UsageDynamic and AccessReadWrite
		MapWriteDiscard,        // gain write access, discard previous content, must be UsageDynamic and AccessWrite
		MapWriteNoOverwrite,    // gain write access, must be UsageDynamic and AccessWrite, see D3D10 docs for details
	};

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

	EType					Type;
	DWORD					Width;
	DWORD					Height;
	DWORD					Depth;
	DWORD					MipCount;
	EPixelFormat			PixelFormat;

	DWORD					LockCount;

	IDirect3DBaseTexture9*	pD3D9Tex;

	union
	{
		IDirect3DTexture9*			pD3D9Tex2D;
		IDirect3DVolumeTexture9*	pD3D9Tex3D;
		IDirect3DCubeTexture9*		pD3D9TexCube;
	};

	void MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags);

public:

	EUsage					Usage;
	EAccessMode				AccessMode;
	//int						SkippedMips;

	CTexture(CStrID ID);
	virtual ~CTexture() { n_assert(!pD3D9Tex); }

	virtual void	Unload();

	bool			Map(int MipLevel, EMapType MapType, CMapInfo& OutMapInfo);
	void			Unmap(int MipLevel);
	bool			MapCubeFace(ECubeFace Face, int MipLevel, EMapType MapType, CMapInfo& OutMapInfo);
	void			UnmapCubeFace(ECubeFace Face, int MipLevel);

	void			SetupFromD3D9Texture(IDirect3DTexture9* pTex, bool SetLoaded = true);
	void			SetupFromD3D9CubeTexture(IDirect3DCubeTexture9* pTex, bool SetLoaded = true);
	void			SetupFromD3D9VolumeTexture(IDirect3DVolumeTexture9* pTex, bool SetLoaded = true);

	EType						GetType() const { return Type; }
	DWORD						GetWidth() const { return Width; }
	DWORD						GetHeight() const { return Height; }
	DWORD						GetDepth() const { return Depth; }
	DWORD						GetMipLevelCount() const { return MipCount; }
	EPixelFormat				GetPixelFormat() const { return PixelFormat; }

	IDirect3DBaseTexture9*		GetD3D9BaseTexture() const { n_assert(!LockCount); return pD3D9Tex; }
	IDirect3DTexture9*			GetD3D9Texture() const;
	IDirect3DCubeTexture9*		GetD3D9CubeTexture() const;
	IDirect3DVolumeTexture9*	GetD3D9VolumeTexture() const;
};

typedef Ptr<CTexture> PTexture;

inline CTexture::CTexture(CStrID ID):
	CResource(ID),
	Usage(UsageImmutable),
	AccessMode(AccessNone),
	Type(InvalidType),
	Width(0),
	Height(0),
	Depth(0),
	MipCount(0),
	//skippedMips(0),
	PixelFormat(InvalidPixelFormat),
	LockCount(0),
	pD3D9Tex(NULL)
{
}
//---------------------------------------------------------------------

}

#endif
