#pragma once
#ifndef __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>
#include <Data/Flags.h>

// A Direct3D9 implementation of a shader constant buffer.

namespace Render
{

class CD3D9ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D9ConstantBuffer);

protected:

	void InternalDestroy();

public:

	CD3D9ConstantBuffer() {}
	virtual ~CD3D9ConstantBuffer() { InternalDestroy(); }

	bool			Create(/*ID3D11Buffer* pCB, ID3D11DeviceContext* pD3DDeviceCtx, bool StoreRAMCopy*/);
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { OK; }

	virtual bool	BeginChanges();
	virtual bool	SetFloat(DWORD Offset, const float* pData, DWORD Count);
	virtual bool	SetInt(DWORD Offset, const int* pData, DWORD Count);
	virtual bool	SetRawData(DWORD Offset, const void* pData, DWORD Size);
	virtual bool	CommitChanges();

	//DWORD			GetSize() const { return SizeInBytes; }
};

typedef Ptr<CD3D9ConstantBuffer> PD3D9ConstantBuffer;

}

#endif
