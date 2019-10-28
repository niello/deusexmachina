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
// This is just a convenience class that hides typical constant buffer management
// strategy inside to reduce complexity and to avoid redundant implementation of
// buffer management in each renderer.

namespace Render
{

class CConstantBufferSet
{
protected:

	struct CConstBufferRecord
	{
		PConstantBuffer	Buffer;
		U32				Flags;	
	};

	PGPUDriver								GPU;
	CDict<HConstantBuffer, CConstBufferRecord> Buffers;

public:

	CConstantBufferSet();
	~CConstantBufferSet();

	bool				SetGPU(PGPUDriver NewGPU);
	bool				RegisterPermanentBuffer(HConstantBuffer Handle, CConstantBuffer& Buffer);
	bool				IsBufferRegistered(HConstantBuffer Handle) const;
	CConstantBuffer*	RequestBuffer(HConstantBuffer Handle, EShaderType Stage);
	bool				CommitChanges();
	void				UnbindAndClear();
};

}

#endif
