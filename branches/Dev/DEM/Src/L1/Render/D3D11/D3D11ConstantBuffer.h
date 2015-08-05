#pragma once
#ifndef __DEM_L1_RENDER_D3D11_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>
#include <Data/Flags.h>

// A Direct3D11 implementation of a shader constant buffer.
// Buffer can be created as a VRAM-only or with a RAM copy. Buffers with
// a RAM copy support partial updates in all cases. VRAM-only buffers
// save memory, but non-mappable ones don't support BeginChanges() +
// Set...() + EndChanges() and, as of DX11, can't be updated partially.
// Use WriteCommitToVRAM() to update non-mappable VRAM-only buffers.

struct ID3D11Buffer;
struct ID3D11DeviceContext;
enum D3D11_USAGE;

namespace Render
{

class CD3D11ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D11ConstantBuffer);

protected:

	enum
	{
		UsesRAMCopy		= 0x01,
		RAMCopyDirty	= 0x02
	};

	ID3D11Buffer*			pBuffer;
	ID3D11DeviceContext*	pD3DCtx;
	char*					pMapped;
	D3D11_USAGE				D3DUsage;
	Data::CFlags			Flags;
	DWORD					SizeInBytes;

	void InternalDestroy();

public:

	CD3D11ConstantBuffer(): pBuffer(NULL), pD3DCtx(NULL), pMapped(NULL) {}
	virtual ~CD3D11ConstantBuffer() { InternalDestroy(); }

	bool			Create(ID3D11Buffer* pCB, ID3D11DeviceContext* pD3DDeviceCtx, bool StoreRAMCopy);
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { return !!pBuffer; }

	virtual bool	BeginChanges();
	virtual bool	SetFloat(DWORD Offset, const float* pData, DWORD Count);
	virtual bool	SetInt(DWORD Offset, const int* pData, DWORD Count);
	virtual bool	SetRawData(DWORD Offset, const void* pData, DWORD Size);
	virtual bool	CommitChanges();

	bool			WriteChangesToRAM(DWORD Offset, const void* pData, DWORD Size);
	bool			WriteCommitToVRAM(const void* pData);

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
	D3D11_USAGE		GetD3DUsage() const { return D3DUsage; }
	DWORD			GetSize() const { return SizeInBytes; }
};

typedef Ptr<CD3D11ConstantBuffer> PD3D11ConstantBuffer;

inline bool CD3D11ConstantBuffer::SetFloat(DWORD Offset, const float* pData, DWORD Count)
{
	return WriteChangesToRAM(Offset, pData, Count * sizeof(float));
}
//---------------------------------------------------------------------

inline bool CD3D11ConstantBuffer::SetInt(DWORD Offset, const int* pData, DWORD Count)
{
	return WriteChangesToRAM(Offset, pData, Count * sizeof(int));
}
//---------------------------------------------------------------------

inline bool CD3D11ConstantBuffer::SetRawData(DWORD Offset, const void* pData, DWORD Size)
{
	return WriteChangesToRAM(Offset, pData, Size);
}
//---------------------------------------------------------------------

inline bool CD3D11ConstantBuffer::WriteChangesToRAM(DWORD Offset, const void* pData, DWORD Size)
{
	n_assert_dbg(pData && Size && pMapped && (Offset + Size <= SizeInBytes));
	//!!!???PERF:?!
	//if (!memcmp(pMapped + Offset, pData, Size)) OK;
	memcpy(pMapped + Offset, pData, Size);
	Flags.Set(RAMCopyDirty);
	OK;
}
//---------------------------------------------------------------------

}

#endif
