#include "HeightfieldShapeOld.h"

#include <IO/BTFile.h>
#include <Data/Buffer.h>
#include <IO/IOServer.h>

namespace Physics
{
__ImplementClass(Physics::CHeightfieldShapeOld, 'HFSO', Physics::CShape);

CHeightfieldShapeOld::CHeightfieldShapeOld():
	CShape(Heightfield),
	pHeights(NULL),
	Width(0),
	Height(0),
	ODEHeightfieldDataID(NULL),
	ODEHeightfieldID(NULL)
{
}
//---------------------------------------------------------------------

CHeightfieldShapeOld::~CHeightfieldShapeOld()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

void CHeightfieldShapeOld::Init(Data::PParams Desc)
{
	InitialTfm.ident(); //??? (was in mangalore, see XML CompositeLoader)
	SetTransform(InitialTfm);
	SetMaterialType(CMaterialTable::StringToMaterialType(Desc->Get<nString>(CStrID("Mtl"), "Soil")));
	SetFileName(Desc->Get<nString>(CStrID("File")));
}
//---------------------------------------------------------------------

bool CHeightfieldShapeOld::Attach(dSpaceID SpaceID)
{
    if (!CShape::Attach(SpaceID)) FAIL;

	n_assert(!pHeights);

	n_assert(FileName.IsValid());

	float WorldW, WorldH;
	if (FileName.CheckExtension("bt"))
	{
		Data::CBuffer Buffer;
		IOSrv->LoadFileToBuffer(FileName, Buffer);
		{
			IO::CBTFile BTFile(Buffer.GetPtr());
			n_assert(BTFile.GetFileSize() == Buffer.GetSize());
			Width = BTFile.GetWidth();
			Height = BTFile.GetHeight();
			WorldW = (float)(BTFile.GetRightExtent() - BTFile.GetLeftExtent());
			WorldH = (float)(BTFile.GetTopExtent() - BTFile.GetBottomExtent());
			pHeights = (float*)n_malloc(BTFile.GetHeightCount() * sizeof(float));
			BTFile.GetHeights(pHeights);
		}
	}
	else if (FileName.CheckExtension("cdlod"))
	{
		//!!!Load from CDLOD heightmap!
		FAIL;
	}
	else
	{
		n_error("CHeightfieldShapeOld: invalid file extension in '%s'", FileName.CStr());
		FAIL;
	}

	// Fix my collide bits, we don't need to collide against other static and disabled entities
	SetCategoryBits(Static);
	SetCollideBits(Dynamic);

	ODEHeightfieldDataID = dGeomHeightfieldDataCreate();

	//!!!can use short* heights directly from data read with dGeomHeightfieldDataBuildShort!
	dGeomHeightfieldDataBuildSingle(ODEHeightfieldDataID,
									pHeights,
									0,				// Don't copy data, use by ref
									WorldW,
									WorldH,
									Width,
									Height,
									1.0f,
									0.0f,
									1.0f,			// Added to bottom to prevent falling through minimal height
									0);				// No infinite wrapping

	ODEHeightfieldID = dCreateHeightfield(0, ODEHeightfieldDataID, 1); //!!!Optional non-placeable!
	AttachGeom(ODEHeightfieldID, SpaceID);

	//???need mass?

	OK;
}
//---------------------------------------------------------------------

void CHeightfieldShapeOld::Detach()
{
	n_assert(IsAttached());
	n_assert(pHeights);

	dGeomHeightfieldDataDestroy(ODEHeightfieldDataID);

	n_free(pHeights);
	pHeights = NULL;

	CShape::Detach();
};
//---------------------------------------------------------------------

void CHeightfieldShapeOld::RenderDebug(const matrix44& ParentTfm)
{
	if (!IsAttached()) return;
	//matrix44 Tfm = Transform * ParentTfm;
	//nGfxServer2::Instance()->DrawShapeIndexedPrimitives(nGfxServer2::TriangleList,
	//													IndexCount / 3,
	//													(vector3*)pVBuffer,
	//													VertexCount,
	//													VertexWidth,
	//													pIBuffer,
	//													nGfxServer2::Index32,
	//													Tfm,
	//													GetDebugVisualizationColor());
}
//---------------------------------------------------------------------

} // namespace Physics
