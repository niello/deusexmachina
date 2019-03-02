#include "ShaderMetadata.h"
#include <Render/ShaderConstant.h>

namespace Render
{
Data::CHandleManager IShaderMetadata::HandleMgr;

PShaderConstant IShaderMetadata::GetConstant(CStrID ID) const
{
	return GetConstant(GetConstantHandle(ID));
}
//---------------------------------------------------------------------

}
