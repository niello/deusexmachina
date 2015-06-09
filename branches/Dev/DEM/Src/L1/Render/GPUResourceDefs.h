#pragma once
#ifndef __DEM_L1_GPU_RESOURCE_DEFS_H__
#define __DEM_L1_GPU_RESOURCE_DEFS_H__

// Flags and other definitions relevant to GPU resources like vertex, index buffers, textures

namespace Render
{
//!!!LOC: my phone is dead and so offline dictionary bases are too. Some words below may be used
//improperly just because I think they are in their places. Hopefully I'll fix mistakes soon.

// Flags that indicate which hardware has which access to this resource data.
// Some combinations may be unsupported by certain rendering APIs, so, implementations must
// consider to satisfy the most of possible features of a set requested.
// Some common usage patterns are:
// GPU_Read				- immutable resources, initialized on creation, the fastest ones for GPU access
// GPU_Read | CPU_Write	- dynamic resources, suitable for a GPU data that is regularly updated by CPU
enum EResourceAccess
{
	CPU_Read	= 0x01,
	CPU_Write	= 0x02,
	GPU_Read	= 0x04,
	GPU_Write	= 0x08
};

enum EMapType
{
	Map_Setup,				// Gain write access for the initial filling of the buffer. Don't misuse!
	Map_Read,				// Gain read access, must be CPU_Read
	Map_Write,				// Gain write access, must be CPU_Write
	Map_ReadWrite,			// Gain read/write access, must be CPU_Read | CPU_Write
	Map_WriteDiscard,		// Gain write access, discard previous content, must be GPU_Read | CPU_Write
	Map_WriteNoOverwrite,	// Gain write access, must be GPU_Read | CPU_Write, see D3D11 docs for details
};

}

#endif
