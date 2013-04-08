#pragma once
#ifndef __DEM_L1_GPU_RESOURCE_DEFS_H__
#define __DEM_L1_GPU_RESOURCE_DEFS_H__

// Flags and other definitions relevant to GPU resources like vertex, index buffers, textures

namespace Render
{

enum EUsage
{
	Usage_Immutable,	// can only be read by GPU, not written, cannot be accessed by CPU
	Usage_Dynamic,		// can only be read by GPU, can only be written by CPU
	Usage_CPU			// a resource which is only accessible by the CPU and can't be used for rendering
};

enum ECPUAccess
{
	CPU_NoAccess,		// CPU does not require access to the resource (best)
	CPU_Read,			// CPU has read access
	CPU_Write,			// CPU has write access
	CPU_ReadWrite		// CPU has read/write access
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
