#pragma once
#ifndef __DEM_L1_RENDER_EFFECT_CONST_BLOCK_H__
#define __DEM_L1_RENDER_EFFECT_CONST_BLOCK_H__

#include <Render/RenderFwd.h>
#include <Data/Dictionary.h>
//#include <Data/FixedArray.h>

// Encapsulates functionality to set effect constant values and to apply constant buffers
// with these values to shaders, along with constant buffer instances used by constants.
// You need to register each constant buffer used, and set values only to constants that
// target one of registered buffers. If buffer is registered with no instance, instance
// will be requested as a temporary buffer from the GPU driver.
// This is just a convenience class created to place all the redundant shader constant logic
// in one module and therefore avoid code duplication.
// An instance of effect constant set. Whereas effect constants store only metadata, this
// class stores actual values and manages constant buffers required for storing them.

namespace Render
{

class CEffectConstSetValues
{
protected:

	enum
	{
		// First ShaderType_COUNT flags are (1 << EShaderType) for target shader stages
		ECSV_TmpBuffer = (1 << ShaderType_COUNT)
	};

	struct CConstBufferRecord
	{
		PConstantBuffer	Buffer;
		U32				Flags;	
	};

	PGPUDriver								GPU;
	CDict<HConstBuffer, CConstBufferRecord> Buffers;

public:

	bool	SetGPU(PGPUDriver NewGPU);
	bool	RegisterConstantBuffer(HConstBuffer Handle, CConstantBuffer* pBuffer);
	bool	IsConstantBufferRegistered(HConstBuffer Handle) { return Buffers.Contains(Handle); }
	bool	SetConstantValue(const CEffectConstant* pConst, UPTR ElementIndex, const void* pValue, UPTR Size);
	bool	ApplyConstantBuffers();
	void	UnbindAndClear();
};

}

#endif
