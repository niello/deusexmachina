#pragma once
#ifndef __DEM_L1_RENDER_CONSTANT_BUFFER_SET_H__
#define __DEM_L1_RENDER_CONSTANT_BUFFER_SET_H__

#include <Render/RenderFwd.h>
#include <Data/Dictionary.h>

// Constant buffer set manages permanent and temporary constant buffers required
// to store shader constant values. Permanent buffers are created once and updated
// with actual values, temporary buffers are created by GPU on demand, filled with
// values, used and then discarded. If there is no permanent buffer registered for
// a particular handle, the temporary one will be created on the first request.
// This is just a conveience class that hides typical constant buffer management
// strategy inside to reduce complexity and to avoid redundant implementation of
// buffer management in each renderer.

namespace Render
{

class CConstantBufferSet
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

	CConstantBufferSet();
	~CConstantBufferSet();

	bool				SetGPU(PGPUDriver NewGPU);
	bool				RegisterPermanentBuffer(HConstBuffer Handle, CConstantBuffer& Buffer);
	bool				IsBufferRegistered(HConstBuffer Handle) const { return Buffers.Contains(Handle); }
	CConstantBuffer*	RequestBuffer(HConstBuffer Handle, EShaderType Stage);
	bool				CommitChanges();
	void				UnbindAndClear();
};

}

#endif
