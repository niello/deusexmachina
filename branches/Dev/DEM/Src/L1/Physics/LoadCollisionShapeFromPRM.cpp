// Loads collision shape from .prm file
// Use function declaration instead of header file where you want to call this loader.

//!!!NEED .bullet FILE LOADER!

//!!!???what if shape is found but is not loaded? RESMGR problem!
//desired way:
//if resource is found, but not loaded
//  pass in into the loader
//if loader determines that the type is incompatible
//  it gets the resource pointer from the resource manager
//  sets it to the passed pointer
//  checks its type
//  if type is right, someone reloaded the resource before
//  else replaces a passed pointer with a new one, of the right type

#include <Physics/HeightfieldShape.h>
#include <Physics/PhysicsServer.h>
#include <Physics/BulletConv.h>
#include <Data/DataServer.h>
#include <Data/Params.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <Math/AABB.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

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
	DEFINE_STRID(CDLODFile)
}

namespace Physics
{

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, Data::CParams& In)
{
	CStrID Type = In.Get<CStrID>(Str::Type);
	if (Type == Str::StaticMesh)
	{
		//Shape = 
		//create btStridingMeshInterface*
		//???can get interface pointer from the shape?
	}
	else if (Type == Str::Heightfield)
	{
		CString FileName = In.Get<CString>(Str::CDLODFile, NULL);
		if (!FileName.IsValid()) return NULL;
		FileName = "Terrain:" + FileName + ".cdlod";

		void* pHFData = NULL;
		DWORD HFWidth, HFHeight;
		float HScale;
		CAABB AABB;

		if (FileName.CheckExtension("cdlod"))
		{
			//!!!DUPLICATE CODE! See Scene::CTerrain!
			IO::CFileStream CDLODFile;
			if (!CDLODFile.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
			IO::CBinaryReader Reader(CDLODFile);

			n_assert(Reader.Read<int>() == 'CDLD');	// Magic
			n_assert(Reader.Read<int>() == 1);		// Version

			Reader.Read(HFWidth);
			Reader.Read(HFHeight);
			Reader.Read<DWORD>(); // PatchSize
			Reader.Read<DWORD>(); // LODCount
			Reader.Read<DWORD>(); // MinMaxDataSize
			Reader.Read(HScale);
			Reader.Read(AABB.vmin.x);
			Reader.Read(AABB.vmin.y);
			Reader.Read(AABB.vmin.z);
			Reader.Read(AABB.vmax.x);
			Reader.Read(AABB.vmax.y);
			Reader.Read(AABB.vmax.z);

			DWORD DataSize = HFWidth * HFHeight * sizeof(unsigned short);
			pHFData = n_malloc(DataSize);
			CDLODFile.Read(pHFData, DataSize);

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

		PHeightfieldShape HFShape = PhysicsSrv->CollisionShapeMgr.CreateTypedResource<CHeightfieldShape>(UID);
		if (HFShape.IsValid())
		{
			btHeightfieldTerrainShape* pBtShape =
				new btHeightfieldTerrainShape(HFWidth, HFHeight, pHFData, HScale, AABB.vmin.y, AABB.vmax.y, 1, PHY_SHORT, false);

			btVector3 LocalScaling((AABB.vmax.x - AABB.vmin.x) / (float)(HFWidth - 1), 1.f, (AABB.vmax.z - AABB.vmin.z) / (float)(HFHeight - 1));
			pBtShape->setLocalScaling(LocalScaling);

			vector3 Offset((AABB.vmax.x - AABB.vmin.x) * 0.5f, (AABB.vmin.y + AABB.vmax.y) * 0.5f, (AABB.vmax.z - AABB.vmin.z) * 0.5f);
			if (HFShape->Setup(pBtShape, pHFData, Offset)) return HFShape.GetUnsafe();

			delete pBtShape;
		}

		n_free(pHFData);
	}
	else
	{
		btCollisionShape* pBtShape;

		if (Type == Str::Sphere)
		{
			pBtShape = new btSphereShape(In.Get<float>(Str::Radius, 1.f));
		}
		else if (Type == Str::Box)
		{
			vector3 Ext = In.Get(Str::Size, vector3(1.f, 1.f, 1.f));
			Ext *= 0.5f;
			pBtShape = new btBoxShape(VectorToBtVector(Ext));
		}
		else if (Type == Str::Capsule)
		{
			pBtShape = new btCapsuleShape(In.Get<float>(Str::Radius, 1.f), In.Get<float>(Str::Height, 1.f));
		}
		else return NULL;

		PCollisionShape Shape = PhysicsSrv->CollisionShapeMgr.CreateTypedResource(UID);
		if (Shape.IsValid() && Shape->Setup(pBtShape)) return Shape;

		delete pBtShape;
	}

	return NULL;
}
//---------------------------------------------------------------------

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const CString& FileName)
{
	Data::PParams Desc = DataSrv->LoadPRM(FileName, false);
	return Desc.IsValid() ? LoadCollisionShapeFromPRM(UID, *Desc): NULL;
}
//---------------------------------------------------------------------

}