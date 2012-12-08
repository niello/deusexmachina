#pragma once
#ifndef __DEM_L1_GPU_RESOURCE_DEFS_H__
#define __DEM_L1_GPU_RESOURCE_DEFS_H__

// Flags and other definitions relevant to GPU resources like vertex, index buffers, textures

namespace Render
{

enum EUsage
{
	UsageImmutable,      //> can only be read by GPU, not written, cannot be accessed by CPU
	UsageDynamic,        //> can only be read by GPU, can only be written by CPU
	UsageCPU            //> a resource which is only accessible by the CPU and can't be used for rendering
};

enum ECPUAccess
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

}

#endif
