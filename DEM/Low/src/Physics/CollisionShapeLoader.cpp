#include "CollisionShapeLoader.h"

#include <Physics/HeightfieldShape.h>
#include <Physics/BulletConv.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Math/AABB.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Core/Factory.h>

#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

//!!!NEED .bullet FILE LOADER!

#define DEFINE_STRID(Str) static const CStrID Str(#Str);

namespace Str
{
	DEFINE_STRID(Type)
	DEFINE_STRID(Sphere)
	DEFINE_STRID(Radius)
	DEFINE_STRID(Box)
	DEFINE_STRID(Size)
	DEFINE_STRID(Capsule)
	DEFINE_STRID(Height)
	DEFINE_STRID(StaticMesh)
	DEFINE_STRID(Heightfield)
	DEFINE_STRID(HeightMapFile)
}

namespace Resources
{
__ImplementClass(Resources::CCollisionShapeLoaderPRM, 'CSLD', Resources::IResourceCreator);

const Core::CRTTI& CCollisionShapeLoaderPRM::GetResultType() const
{
	return Physics::CCollisionShape::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CCollisionShapeLoaderPRM::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);
	Data::PParams Desc = n_new(Data::CParams);
	if (!Reader.ReadParams(*Desc)) return NULL;

	CStrID Type = Desc->Get<CStrID>(Str::Type);
	if (Type == Str::StaticMesh)
	{
		//Shape = 
		//create btStridingMeshInterface*
		//???can get interface pointer from the shape?
		NOT_IMPLEMENTED;
		return NULL;
	}
	else if (Type == Str::Heightfield)
	{
		CString FileName = Desc->Get<CString>(Str::HeightMapFile, CString::Empty);
		if (!FileName.IsValid()) return NULL;
		FileName = "Terrain:" + FileName + ".cdlod";

		void* pHFData = NULL;
		U32 HFWidth, HFHeight;
		float HScale;
		CAABB AABB;

		if (PathUtils::CheckExtension(FileName, "cdlod"))
		{
			//!!!DUPLICATE CODE! See CCDLODDataLoader!
			IO::PStream CDLODFile = IOSrv->CreateStream(FileName);
			if (!CDLODFile->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return NULL;
			IO::CBinaryReader Reader(*CDLODFile);

			n_assert(Reader.Read<U32>() == 'CDLD');	// Magic
			n_assert(Reader.Read<U32>() == 1);		// Version

			Reader.Read(HFWidth);
			Reader.Read(HFHeight);
			Reader.Read<U32>(); // PatchSize
			Reader.Read<U32>(); // LODCount
			Reader.Read<U32>(); // MinMaxDataSize
			Reader.Read(HScale);
			Reader.Read(AABB.Min.x);
			Reader.Read(AABB.Min.y);
			Reader.Read(AABB.Min.z);
			Reader.Read(AABB.Max.x);
			Reader.Read(AABB.Max.y);
			Reader.Read(AABB.Max.z);

			UPTR DataSize = HFWidth * HFHeight * sizeof(U16);
			pHFData = n_malloc(DataSize);
			CDLODFile->Read(pHFData, DataSize);

			// Convert to signed
			const unsigned short* pUData = ((unsigned short*)pHFData);
			const unsigned short* pUDataEnd = ((unsigned short*)pHFData) + HFWidth * HFHeight;
			short* pSData = ((short*)pHFData);
			while (pUData < pUDataEnd)
			{
				*pSData = *pUData - 32767;
				++pUData;
				++pSData;
			}
		}
		else return NULL;

		Physics::PHeightfieldShape HFShape = n_new(Physics::CHeightfieldShape);
		if (HFShape.IsValidPtr())
		{
			btHeightfieldTerrainShape* pBtShape =
				new btHeightfieldTerrainShape(HFWidth, HFHeight, pHFData, HScale, AABB.Min.y, AABB.Max.y, 1, PHY_SHORT, false);

			btVector3 LocalScaling((AABB.Max.x - AABB.Min.x) / (float)(HFWidth - 1), 1.f, (AABB.Max.z - AABB.Min.z) / (float)(HFHeight - 1));
			pBtShape->setLocalScaling(LocalScaling);

			vector3 Offset((AABB.Max.x - AABB.Min.x) * 0.5f, (AABB.Min.y + AABB.Max.y) * 0.5f, (AABB.Max.z - AABB.Min.z) * 0.5f);

			if (HFShape->Setup(pBtShape, pHFData, Offset)) return HFShape.Get();

			delete pBtShape;
		}

		n_free(pHFData);
	}
	else
	{
		btCollisionShape* pBtShape;

		if (Type == Str::Sphere)
		{
			pBtShape = new btSphereShape(Desc->Get<float>(Str::Radius, 1.f));
		}
		else if (Type == Str::Box)
		{
			vector3 Ext = Desc->Get(Str::Size, vector3(1.f, 1.f, 1.f));
			Ext *= 0.5f;
			pBtShape = new btBoxShape(VectorToBtVector(Ext));
		}
		else if (Type == Str::Capsule)
		{
			pBtShape = new btCapsuleShape(Desc->Get<float>(Str::Radius, 1.f), Desc->Get<float>(Str::Height, 1.f));
		}
		else return NULL;

		Physics::PCollisionShape Shape = n_new(Physics::CCollisionShape);
		if (Shape.IsValidPtr() && Shape->Setup(pBtShape)) return Shape.Get();

		delete pBtShape;
	}

	return NULL;
}
//---------------------------------------------------------------------

}