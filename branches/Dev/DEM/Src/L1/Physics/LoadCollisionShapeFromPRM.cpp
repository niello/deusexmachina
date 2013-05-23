// Loads collision shape from .prm file
// Use function declaration instead of header file where you want to call this loader.

//!!!NEED .bullet FILE LOADER!

#include <Physics/CollisionShape.h>
#include <Physics/PhysicsServer.h>
#include <Data/DataServer.h>
#include <Data/Params.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

//!!!later migrate to bullet math!
inline vector3 BtVectorToVector(const btVector3& Vec) { return vector3(Vec.x(), Vec.y(), Vec.z()); }
inline btVector3 VectorToBtVector(const vector3& Vec) { return btVector3(Vec.x, Vec.y, Vec.z); }

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
	DEFINE_STRID(Mesh)
	DEFINE_STRID(Heightfield)
	DEFINE_STRID(FileName)
}

namespace Physics
{

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, Data::CParams& In)
{
	PCollisionShape Shape;
	btCollisionShape* pBtShape;

	CStrID Type = In.Get<CStrID>(Str::Type);
	if (Type == Str::Sphere)
	{
		pBtShape = new btSphereShape(In.Get<float>(Str::Radius, 1.f));
	}
	else if (Type == Str::Box)
	{
		vector3 Ext = In.Get<vector4>(Str::Size, vector4(1.f, 1.f, 1.f, 0.f));
		Ext *= 0.5f;
		pBtShape = new btBoxShape(VectorToBtVector(Ext));
	}
	else if (Type == Str::Capsule)
	{
		pBtShape = new btCapsuleShape(In.Get<float>(Str::Radius, 1.f), In.Get<float>(Str::Height, 1.f));
	}
	else if (Type == Str::Mesh)
	{
		//Shape = 
		//create btStridingMeshInterface*
		//???can get interface pointer from the shape?
	}
	else if (Type == Str::Heightfield)
	{
		const nString& FileName = In.Get<nString>(Str::FileName, NULL);
		if (!FileName.IsValid()) return NULL;

		void* pHFData = NULL;
		DWORD HFWidth, HFHeight;
		float HScale, MinH, MaxH;

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
			Reader.Read<float>(); //Box.vmin.x
			Reader.Read(MinH);
			Reader.Read<float>(); //Box.vmin.z
			Reader.Read<float>(); //Box.vmax.x
			Reader.Read(MaxH);
			Reader.Read<float>(); //Box.vmax.z

			//???how Bullet determines HF texel to unit (meter) relationship?

			DWORD DataSize = HFWidth * HFHeight * sizeof(unsigned short);
			pHFData = n_malloc(DataSize);
			CDLODFile.Read(pHFData, DataSize);

			//???convert to signed?
		}
		else return NULL;

		//???
		const bool FlipQuadEdges = false;

		//!!!float HFShape->GetHeightCompensationOffset();! bullet shifts the center!
		//PHeightfieldShape HFShape = PhysicsSrv->CollShapeMgr.CreateTypedResource<CHeightfieldShape>(UID);
		pBtShape = new btHeightfieldTerrainShape(HFWidth, HFHeight, pHFData, HScale, MinH, MaxH, 1, PHY_SHORT, FlipQuadEdges);
		//if (HFShape->Setup(pBtShape, pHFData)) return HFShape;

		delete pBtShape;
		return NULL;
	}
	else return NULL;

	if (!Shape.IsValid()) Shape = PhysicsSrv->CollShapeMgr.CreateTypedResource(UID);
	if (!Shape.IsValid()) return NULL;

	if (Shape->Setup(pBtShape)) return Shape;

	delete pBtShape;
	return NULL;
}
//---------------------------------------------------------------------

PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const nString& FileName)
{
	Data::PParams Desc = DataSrv->LoadPRM(FileName, false);
	return Desc.IsValid() ? LoadCollisionShapeFromPRM(UID, *Desc): NULL;
}
//---------------------------------------------------------------------

}