// Loads collision shape from .prm file
// Use function declaration instead of header file where you want to call this loader.

//!!!NEED .bullet FILE LOADER!

#include <Physics/CollisionShape.h>
#include <Data/DataServer.h>
#include <Data/Params.h>

namespace Physics
{

bool LoadCollisionShapeFromPRM(Data::CParams& In, PCollisionShape OutShape)
{
	if (!OutShape.IsValid()) FAIL;

	btCollisionShape* pShape = NULL;

	return OutShape->Setup(pShape);
}
//---------------------------------------------------------------------

bool LoadCollisionShapeFromPRM(const nString& FileName, PCollisionShape OutShape)
{
	Data::PParams Desc = DataSrv->LoadPRM(FileName, false);
	return Desc.IsValid() && LoadCollisionShapeFromPRM(*Desc, OutShape);
}
//---------------------------------------------------------------------

}