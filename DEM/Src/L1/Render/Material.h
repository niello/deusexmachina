#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Resources/ResourceObject.h>

// Material defines parameters of an effect to achieve some visual properties of object rendered.
// Material can be shared among render objects, providing the same shaders and shader parameters to all of them.

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;

class CMaterial: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

public:

	//virtual ~CMaterial();

	virtual bool IsResourceValid() const { FAIL; }
};

}

#endif
