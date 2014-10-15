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
// consider to satisfy the most possible features of a set requested.
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
	Map_Setup,				// gain write access for the initial filling of the buffer. Don't misuse!
	Map_Read,				// gain read access, must be UsageDynamic and AccessRead
	Map_Write,				// gain write access, must be UsageDynamic and AccessWrite
	Map_ReadWrite,			// gain read/write access, must be UsageDynamic and AccessReadWrite
	Map_WriteDiscard,		// gain write access, discard previous content, must be UsageDynamic and AccessWrite
	Map_WriteNoOverwrite,	// gain write access, must be UsageDynamic and AccessWrite, see D3D10 docs for details
};

}

#endif
