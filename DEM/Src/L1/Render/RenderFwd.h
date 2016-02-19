#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <Data/Ptr.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>
#include <Math/AABB.h>

// Render system definitions and forward declarations

#define DEM_RENDER_DEBUG (1)
#define DEM_RENDER_USENVPERFHUD (0)

namespace Render
{
struct CTextureDesc;
struct CSwapChainDesc;
struct CRenderStateDesc;
struct CSamplerDesc;
class CDisplayMode;
typedef Ptr<class CDisplayDriver> PDisplayDriver;
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;
typedef Ptr<class CIndexBuffer> PIndexBuffer;
typedef Ptr<class CRenderTarget> PRenderTarget;
typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;
typedef Ptr<class CRenderState> PRenderState;
typedef Ptr<class CShader> PShader;
typedef Ptr<class CConstantBuffer> PConstantBuffer;
typedef Ptr<class CTexture> PTexture;
typedef Ptr<class CSampler> PSampler;

const UPTR Adapter_AutoSelect = (UPTR)-2;
const UPTR Adapter_None = (UPTR)-1;
const UPTR Adapter_Primary = 0;
const UPTR Adapter_Secondary = 1;
const UPTR Output_None = (UPTR)-1;

// Binding handlers for shader parameters
typedef HHandle HConst;
typedef HHandle HConstBuffer;
typedef HHandle HResource;
typedef HHandle HSampler;

// Named data flow conventions between CPU and GPU, see Renderer.cpp for implementation and details
UPTR RegisterShaderInputSetID(CStrID InputSetName);

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

enum EShaderType
{
	ShaderType_Vertex = 0,	// Don't change order and starting index
	ShaderType_Pixel,
	ShaderType_Geometry,
	ShaderType_Hull,
	ShaderType_Domain,

	ShaderType_COUNT,
	ShaderType_Invalid,
	ShaderType_Unknown = ShaderType_Invalid
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
	Prim_PointList,
	Prim_LineList,
	Prim_LineStrip,
	Prim_TriList,
	Prim_TriStrip,
	Prim_Invalid
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

enum EColorMask
{
	ColorMask_Red	= 0x01,
	ColorMask_Green	= 0x02,
	ColorMask_Blue	= 0x04,
	ColorMask_Alpha	= 0x08
};

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
	CubeFace_PosX = 0,
	CubeFace_NegX = 1,
	CubeFace_PosY = 2,
	CubeFace_NegY = 3,
	CubeFace_PosZ = 4,
	CubeFace_NegZ = 5
};

// Flags that indicate which hardware has which access to this resource data.
// Some combinations may be unsupported by certain rendering APIs, so, implementations must
// consider to satisfy the most of possible features of a set requested.
// Some common usage patterns are:
// Access_GPU_Read						- immutable resources, initialized on creation, the fastest ones for GPU access
// Access_GPU_Read | Access_CPU_Write	- dynamic resources, suitable for a GPU data that is regularly updated by CPU
// Access_GPU_Read | Access_GPU_Write	- texture render targets or very infrequently updated VRAM resources
enum EResourceAccess
{
	Access_CPU_Read		= 0x01,
	Access_CPU_Write	= 0x02,
	Access_GPU_Read		= 0x04,
	Access_GPU_Write	= 0x08
};

enum EResourceMapMode
{
	Map_Read,				// Gain read access, must be Access_CPU_Read
	Map_Write,				// Gain write access, must be Access_CPU_Write
	Map_ReadWrite,			// Gain read/write access, must be Access_CPU_Read | Access_CPU_Write
	Map_WriteDiscard,		// Gain write access, discard previous content, must be Access_GPU_Read | Access_CPU_Write (dynamic)
	Map_WriteNoOverwrite,	// Gain write access, must be Access_GPU_Read | Access_CPU_Write (dynamic)
};

enum ECmpFunc //???to core enums?
{
	Cmp_Never,
	Cmp_Less,
	Cmp_LessEqual,
	Cmp_Greater,
	Cmp_GreaterEqual,
	Cmp_Equal,
	Cmp_NotEqual,
	Cmp_Always
};

enum EStencilOp
{
	StencilOp_Keep,
	StencilOp_Zero,
	StencilOp_Replace,
	StencilOp_Inc,
	StencilOp_IncSat,
	StencilOp_Dec,
	StencilOp_DecSat,
	StencilOp_Invert
};

enum EBlendArg
{
	BlendArg_Zero,
	BlendArg_One,
	BlendArg_SrcColor,
	BlendArg_InvSrcColor,
	BlendArg_Src1Color,
	BlendArg_InvSrc1Color,
	BlendArg_SrcAlpha,
	BlendArg_SrcAlphaSat,
	BlendArg_InvSrcAlpha,
	BlendArg_Src1Alpha,
	BlendArg_InvSrc1Alpha,
	BlendArg_DestColor,
	BlendArg_InvDestColor,
	BlendArg_DestAlpha,
	BlendArg_InvDestAlpha,
	BlendArg_BlendFactor,
	BlendArg_InvBlendFactor
};

enum EBlendOp
{
	BlendOp_Add,
	BlendOp_Sub,
	BlendOp_RevSub,
	BlendOp_Min,
	BlendOp_Max
};

enum ETexAddressMode
{
	TexAddr_Wrap,
	TexAddr_Mirror,
	TexAddr_Clamp,
	TexAddr_Border,
	TexAddr_MirrorOnce
};

enum ETexFilter
{
	TexFilter_MinMagMip_Point,
	TexFilter_MinMag_Point_Mip_Linear,
	TexFilter_Min_Point_Mag_Linear_Mip_Point,
	TexFilter_Min_Point_MagMip_Linear,
	TexFilter_Min_Linear_MagMip_Point,
	TexFilter_Min_Linear_Mag_Point_Mip_Linear,
	TexFilter_MinMag_Linear_Mip_Point,
	TexFilter_MinMagMip_Linear,
	TexFilter_Anisotropic
};

struct CImageData
{
	char*	pData;			// Data sequentially placed in memory
	UPTR	RowPitch;		// Distance in bytes between first bytes of two rows (undefined for 1D)
	UPTR	SlicePitch;		// Distance in bytes between first bytes of two depth slices (undefined for 1D & 2D)
};

struct CRenderTargetDesc
{
	UPTR			Width;
	UPTR			Height;
	EPixelFormat	Format;
	EMSAAQuality	MSAAQuality;
	UPTR			MipLevels;			// Has meaning only for texture RTs (UseAsShaderInput = true, not depth-stencil)
	bool			UseAsShaderInput;
};

struct CViewport
{
	float	Left;
	float	Top;
	float	Width;
	float	Height;
	float	MinDepth;
	float	MaxDepth;
};

struct CPrimitiveGroup
{
	UPTR				FirstVertex;
	UPTR				VertexCount;
	UPTR				FirstIndex;
	UPTR				IndexCount;
	EPrimitiveTopology	Topology;
	CAABB				AABB;
};

// Error codes
#define ERR_CREATION_ERROR ((UPTR)-1);
#define ERR_DRIVER_TYPE_NOT_SUPPORTED ((UPTR)-2);

inline UPTR GetMipLevelCount(UPTR Width, UPTR Height, UPTR BlockSize = 1)
{
	UPTR MaxDim = n_max(Width, Height);
	UPTR MipLevels = 1;
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
