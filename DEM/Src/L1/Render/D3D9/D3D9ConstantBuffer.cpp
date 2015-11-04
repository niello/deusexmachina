#include "D3D9ConstantBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9ConstantBuffer, 'CB09', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D9ConstantBuffer::Create(/*pfloatregs, floatcount, pintregs, intcount*/)
{
	//if (!pCB || !pD3DDeviceCtx) FAIL;

	//!!!allocate one chunk of memory and setup pointers!
	//sort by register, if !sorted
	//can check if sorted or just always sort

	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::InternalDestroy()
{
	if (pFloat4Data)
	{
		n_free(pFloat4Data);
		pFloat4Data = NULL;
	}
	else if (pInt4Data)
	{
		n_free(pInt4Data);
	}
	pFloat4Registers = NULL;
	Float4Count = 0;
	pInt4Data = NULL;
	pInt4Registers = NULL;
	Int4Count = 0;
}
//---------------------------------------------------------------------

}
