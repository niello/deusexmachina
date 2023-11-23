#pragma once
#include <Render/IndexBuffer.h>

// Direct3D9 implementation of an index buffer

struct IDirect3DIndexBuffer9;
typedef unsigned int UINT;

namespace Render
{

class CD3D9IndexBuffer: public CIndexBuffer
{
	FACTORY_CLASS_DECL;

protected:

	IDirect3DIndexBuffer9*	pBuffer = nullptr;
	UINT					Usage = 0;
	//UPTR					LockCount = 0;

	void InternalDestroy();

public:

	virtual ~CD3D9IndexBuffer() { InternalDestroy(); }

	bool					Create(EIndexType Type, IDirect3DIndexBuffer9* pIB);
	virtual void			Destroy() { InternalDestroy(); CIndexBuffer::Destroy(); }
	virtual bool			IsValid() const { return !!pBuffer; }
	virtual void            SetDebugName(std::string_view Name) override;

	IDirect3DIndexBuffer9*	GetD3DBuffer() const { return pBuffer; }
	UINT					GetD3DUsage() const { return Usage; }
};

typedef Ptr<CD3D9IndexBuffer> PD3D9IndexBuffer;

}
