#pragma once
#ifndef __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>
#include <Data/Flags.h>

// A Direct3D9 implementation of a shader constant buffer.

struct IDirect3DDevice9;

namespace Render
{

class CD3D9ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D9ConstantBuffer);

protected:

	enum { RegisterDataIsDirty = 0x80000000 };

	//!!!can allocate all data as one block of memory and save offsets!
	float*				pFloatData;
	DWORD*				pFloatRegisters;
	DWORD				FloatCount;

	int*				pIntData;
	DWORD*				pIntRegisters;
	DWORD				IntCount;

	IDirect3DDevice9*	pDevice;

	void InternalDestroy();

public:

	CD3D9ConstantBuffer(): pDevice(NULL), pFloatData(NULL), pIntData(NULL) {}
	virtual ~CD3D9ConstantBuffer() { InternalDestroy(); }

	bool			Create();
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { return !!pDevice; }

	virtual bool	BeginChanges();
	virtual bool	SetFloat(DWORD Offset, const float* pData, DWORD Count);
	virtual bool	SetInt(DWORD Offset, const int* pData, DWORD Count);
	virtual bool	SetRawData(DWORD Offset, const void* pData, DWORD Size) { FAIL; } // Not supported
	virtual bool	CommitChanges();
};

typedef Ptr<CD3D9ConstantBuffer> PD3D9ConstantBuffer;

}

#endif
