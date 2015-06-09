#pragma once
#ifndef __DEM_L1_D3D9_FWD_H__
#define __DEM_L1_D3D9_FWD_H__

// Forward declarations of D3D9

#include <StdDEM.h> // To declare LPCSTR etc

// Macros to map unsigned 8 bits/channel to D3DCOLOR
#define N_ARGB(a, r, g, b)			((uint)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))
#define N_RGBA(r, g, b, a)			N_ARGB(a, r, g, b)
#define N_XRGB(r, g, b)				N_ARGB(0xff, r, g, b)
#define N_COLORVALUE(r, g, b, a)	N_RGBA((uint)((r)*255.f), (uint)((g)*255.f), (uint)((b)*255.f), (uint)((a)*255.f))

//#define DEM_USE_D3DX9

typedef enum _D3DFORMAT D3DFORMAT;
typedef D3DFORMAT EPixelFormat;
#define PixelFormat_Invalid ((EPixelFormat)0) // D3DFMT_UNKNOWN

typedef enum _D3DXIMAGE_FILEFORMAT D3DXIMAGE_FILEFORMAT;
typedef D3DXIMAGE_FILEFORMAT EImageFormat;

typedef enum _D3DMULTISAMPLE_TYPE D3DMULTISAMPLE_TYPE;

#ifndef D3DXFX_LARGEADDRESS_HANDLE
typedef LPCSTR D3DXHANDLE;
#else
typedef UINT_PTR D3DXHANDLE;
#endif

struct IDirect3D9;
struct IDirect3DDevice9;

struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexDeclaration9;

struct IDirect3DSurface9;
struct IDirect3DBaseTexture9;
struct IDirect3DTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;

struct ID3DXEffect;
struct ID3DXEffectPool;

struct ID3DXFont;
struct ID3DXSprite;

#endif
