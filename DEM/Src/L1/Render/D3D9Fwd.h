#pragma once
#ifndef __DEM_L1_D3D9_FWD_H__
#define __DEM_L1_D3D9_FWD_H__

// Forward declarations of D3D9

#include <StdDEM.h> // To declare LPCSTR etc

#define DEM_D3D_DEBUG (0)
#define DEM_D3D_USENVPERFHUD (0)

#if DEM_D3D_USENVPERFHUD
#define DEM_D3D_DEVICETYPE D3DDEVTYPE_REF
#else
#define DEM_D3D_DEVICETYPE D3DDEVTYPE_HAL
#endif

typedef enum _D3DFORMAT D3DFORMAT;
typedef D3DFORMAT EPixelFormat;
#define PixelFormat_Invalid ((D3DFORMAT)0) // D3DFMT_UNKNOWN

typedef enum _D3DMULTISAMPLE_TYPE D3DMULTISAMPLE_TYPE;

#ifndef D3DXFX_LARGEADDRESS_HANDLE
typedef LPCSTR D3DXHANDLE;
#else
typedef UINT_PTR D3DXHANDLE;
#endif

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

#endif
