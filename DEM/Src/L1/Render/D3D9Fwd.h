#pragma once
#ifndef __DEM_L1_D3D9_FWD_H__
#define __DEM_L1_D3D9_FWD_H__

// Forward declarations of D3D9

typedef enum _D3DFORMAT D3DFORMAT;
typedef D3DFORMAT EPixelFormat;
#define InvalidPixelFormat ((D3DFORMAT)0)
#define AsNebulaPixelFormat

#ifndef D3DXFX_LARGEADDRESS_HANDLE
typedef LPCSTR D3DXHANDLE;
#else
typedef UINT_PTR D3DXHANDLE;
#endif

struct IDirect3DDevice9;

struct IDirect3DBaseTexture9;
struct IDirect3DTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;

struct ID3DXEffect;
struct ID3DXEffectPool;

#endif
