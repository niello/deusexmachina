#pragma once
#ifndef __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>
//#include <Data/Flags.h>

// A Direct3D9 implementation of a shader constant buffer.

namespace Render
{

class CD3D9ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D9ConstantBuffer);

protected:

	//enum { RegisterDataIsDirty = 0x80000000 };

	float*				pFloat4Data;
	DWORD*				pFloat4Registers;
	DWORD				Float4Count;

	int*				pInt4Data;
	DWORD*				pInt4Registers;
	DWORD				Int4Count;

	//BOOL*				pBoolData;
	//DWORD*				pBoolRegisters;
	//DWORD				BoolCount;

	void InternalDestroy();

public:

	CD3D9ConstantBuffer(): pFloat4Data(NULL), pInt4Data(NULL) {}
	virtual ~CD3D9ConstantBuffer() { InternalDestroy(); }

	bool			Create(/**/);
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { return pFloat4Data || pInt4Data /*|| pBoolData*/; }
};

typedef Ptr<CD3D9ConstantBuffer> PD3D9ConstantBuffer;

}

#endif
