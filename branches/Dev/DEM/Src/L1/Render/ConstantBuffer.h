#pragma once
#ifndef __DEM_L1_RENDER_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_CONSTANT_BUFFER_H__

#include <Core/Object.h>

// A hardware GPU buffer that contains shader uniform constants

namespace Render
{

class CConstantBuffer: public Core::CObject
{
protected:

public:

	virtual void	Destroy() = 0;
	virtual bool	IsValid() const = 0;

	virtual bool	BeginChanges() = 0;
	virtual bool	SetFloat(DWORD Offset, const float* pData, DWORD Count) = 0; //???Offset or HConst Address?
	virtual bool	SetInt(DWORD Offset, const int* pData, DWORD Count) = 0; //???Offset or HConst Address?
	virtual bool	SetRawData(DWORD Offset, const void* pData, DWORD Size) = 0; //???Offset or HConst Address?
	virtual bool	CommitChanges() = 0;
};

typedef Ptr<CConstantBuffer> PConstantBuffer;

}

#endif
