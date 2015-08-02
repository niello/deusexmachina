#pragma once
#ifndef __DEM_L1_RENDER_D3D11_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>

// A Direct3D11 implementation of a shader constant buffer

struct ID3D11Buffer;
enum D3D11_USAGE;

namespace Render
{

class CD3D11ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D11ConstantBuffer);

protected:

	ID3D11Buffer*	pBuffer;
	//???store access in a parent class?
	//???store size in a parent class?
	D3D11_USAGE		D3DUsage;

	void InternalDestroy();

public:

	CD3D11ConstantBuffer(): pBuffer(NULL) {}
	virtual ~CD3D11ConstantBuffer() { InternalDestroy(); }

	bool			Create(ID3D11Buffer* pCB);
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { return !!pBuffer; }

	// set value to some offset/register

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
	D3D11_USAGE		GetD3DUsage() const { return D3DUsage; }
};

typedef Ptr<CD3D11ConstantBuffer> PD3D11ConstantBuffer;

}

#endif
