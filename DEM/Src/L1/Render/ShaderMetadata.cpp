#include "ShaderMetadata.h"

namespace Render
{
Data::CHandleManager IShaderMetadata::HandleMgr;

void* IShaderMetadata::GetHandleData(HHandle Handle)
{
	//???implement some type safety?
	return HandleMgr.GetHandleData(Handle);
}
//---------------------------------------------------------------------

}
