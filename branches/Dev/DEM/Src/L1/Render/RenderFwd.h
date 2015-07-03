#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <Data/Ptr.h>

// Render system definitions and forward declarations

#define DEM_RENDER_DEBUG (1)
#define DEM_RENDER_USENVPERFHUD (0)

namespace Render
{
//typedef DWORD HShaderParam; // Opaque to user, so its internal meaning can be different for different APIs
//struct CShaderConstantDesc;
struct CTextureDesc;
struct CSwapChainDesc;
class CDisplayMode;
typedef Ptr<class CDisplayDriver> PDisplayDriver;
typedef Ptr<class CTexture> PTexture;
typedef Ptr<class CRenderTarget> PRenderTarget;
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;
typedef Ptr<class CIndexBuffer> PIndexBuffer;
typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;
typedef Ptr<class CRenderState> PRenderState;
typedef Ptr<class CShader> PShader;

const DWORD Adapter_AutoSelect = (DWORD)-2;
const DWORD Adapter_None = (DWORD)-1;
const DWORD Adapter_Primary = 0;
const DWORD Adapter_Secondary = 1;
const DWORD Output_None = (DWORD)-1;

enum EGPUDriverType
{
	// Prefers hardware driver when possible and falls back to reference device.
	// Use it only as a creation parameter and never as an actual driver type.
	GPU_AutoSelect = 0,

	GPU_Hardware,	// Real hardware device
	GPU_Reference,	// Software emulation (for debug purposes)
	GPU_Software,	// Pluggable software driver
	GPU_Null		// No rendering (for non-rendering API calls verification)
};

enum EClearFlag
{
	Clear_Color		= 0x01,
	Clear_Depth		= 0x02,
	Clear_Stencil	= 0x04,
	Clear_All		= (Clear_Color | Clear_Depth | Clear_Stencil)
};

enum EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriList,
	TriStrip
};

// Value is a size of one index in bytes
enum EIndexType
{
	Index_16 = 2,
	Index_32 = 4
};

enum ECaps
{
	Caps_VSTex_L16,				// Unsigned short 16-bit texture as a vertex texture
	Caps_VSTexFiltering_Linear,	// Bilinear min & mag filtering for vertex textures
	Caps_ReadDepthAsTexture		// Use depth buffer as a shader input
};

//!!!fill!
//???reverse component order? see DXGI formats
enum EPixelFormat
{
	PixelFmt_Invalid = 0,
	PixelFmt_DefaultBackBuffer,
	PixelFmt_DefaultDepthBuffer,
	PixelFmt_DefaultDepthStencilBuffer,
	PixelFmt_R8G8B8X8,
	PixelFmt_R8G8B8A8,
	PixelFmt_B8G8R8X8,
	PixelFmt_B8G8R8A8,
	PixelFmt_B5G6R5,
	PixelFmt_DXT1,
	PixelFmt_DXT3,
	PixelFmt_DXT5,
	PixelFmt_D24,
	PixelFmt_D24S8,
	PixelFmt_D32,
	PixelFmt_D32S8
};

enum EMSAAQuality
{
	MSAA_None	= 1,
	MSAA_2x		= 2,
	MSAA_4x		= 4,
	MSAA_8x		= 8
};

enum ETextureType
{
	Texture_1D,
	Texture_2D,
	Texture_3D,
	Texture_Cube
};

enum ECubeMapFace
{
	PosX = 0,
	NegX,
	PosY,
	NegY,
	PosZ,
	NegZ
};

// Flags that indicate which hardware has which access to this resource data.
// Some combinations may be unsupported by certain rendering APIs, so, implementations must
// consider to satisfy the most of possible features of a set requested.
// Some common usage patterns are:
// GPU_Read				- immutable resources, initialized on creation, the fastest ones for GPU access
// GPU_Read | CPU_Write	- dynamic resources, suitable for a GPU data that is regularly updated by CPU
enum EResourceAccess
{
	Access_CPU_Read		= 0x01,
	Access_CPU_Write	= 0x02,
	Access_GPU_Read		= 0x04,
	Access_GPU_Write	= 0x08
};

enum EMapType
{
	Map_Read,				// Gain read access, must be CPU_Read
	Map_Write,				// Gain write access, must be CPU_Write
	Map_ReadWrite,			// Gain read/write access, must be CPU_Read | CPU_Write
	Map_WriteDiscard,		// Gain write access, discard previous content, must be GPU_Read | CPU_Write
	Map_WriteNoOverwrite,	// Gain write access, must be GPU_Read | CPU_Write, see D3D11 docs for details
};

struct CRenderTargetDesc
{
	DWORD			Width;
	DWORD			Height;
	EPixelFormat	Format;
	EMSAAQuality	MSAAQuality;
	DWORD			MipLevels;			// Has meaning only for texture RTs (UseAsShaderInput = true, not depth-stencil)
	bool			UseAsShaderInput;
};

struct CViewport
{
	//???use CRectT<float, float> for L, T, W, H?
	float	Left;
	float	Top;
	float	Width;
	float	Height;
	float	MinDepth;
	float	MaxDepth;
};

// Error codes
#define ERR_CREATION_ERROR ((DWORD)-1);
#define ERR_DRIVER_TYPE_NOT_SUPPORTED ((DWORD)-2);

inline DWORD GetMipLevelCount(DWORD Width, DWORD Height, DWORD BlockSize = 1)
{
	DWORD MaxDim = n_max(Width, Height);
	DWORD MipLevels = 1;
	while (MaxDim > BlockSize)
	{
		MaxDim >>= 1;
		++MipLevels;
	}
	return MipLevels;
}
//---------------------------------------------------------------------

}

#endif
