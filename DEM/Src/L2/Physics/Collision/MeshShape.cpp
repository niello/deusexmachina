#include "MeshShape.h"

namespace Physics
{
__ImplementClass(Physics::CMeshShape, 'MSSH', Physics::CShape);

CMeshShape::CMeshShape():
	CShape(Mesh),
	pVBuffer(NULL),
	pIBuffer(NULL),
	VertexCount(0),
	VertexWidth(0),
	IndexCount(0),
	ODETriMeshDataID(NULL),
	ODETriMeshID(NULL)
{
	//!!!no need to init in each mesh shape!

	//// initialize the OPCODE sphere collider
	//OPCSphereCollider.SetFirstContact(false);		// report all contacts
	//OPCSphereCollider.SetTemporalCoherence(true);	// use temporal coherence

	//// initialize the OPCODE ray collider
	//OPCRayCollider.SetFirstContact(false);			// report all contacts
	//OPCRayCollider.SetTemporalCoherence(false);		// no temporal coherence
	//OPCRayCollider.SetClosestHit(true);				// return only closest hit
	//OPCRayCollider.SetCulling(true);				// with backface culling
	//OPCRayCollider.SetDestination(&OPCFaces);		// detected hits go here
}
//---------------------------------------------------------------------

CMeshShape::~CMeshShape()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

void CMeshShape::Init(Data::PParams Desc)
{
	InitialTfm.ident(); //??? (was in mangalore, see XML CompositeLoader)
	SetTransform(InitialTfm);
	SetMaterialType(CMaterialTable::StringToMaterialType(Desc->Get<nString>(CStrID("Mtl"), "Metal")));
	SetFileName(Desc->Get<nString>(CStrID("File")));
}
//---------------------------------------------------------------------

// Create a mesh shape object, add it to ODE's collision space, and initialize the mass member.
bool CMeshShape::Attach(dSpaceID SpaceID)
{
    if (!CShape::Attach(SpaceID)) FAIL;

	n_assert(!pVBuffer);
	n_assert(!pIBuffer);

	//!!!REVISIT IT! Now it's copypaste from A. Sysoev's code
	if (NULL) //pInitMesh)
	{
		/*
		// load the vertices and indices
		VertexCount = pInitMesh->GetNumVertices();
		IndexCount  = 3 * pInitMesh->GetNumIndices() - 6;
		VertexWidth = pInitMesh->GetVertexWidth();

		// allocate vertex and index buffer
		int VBSize = pInitMesh->GetVertexBufferByteSize();
		int IBSize = IndexCount * sizeof(int);
		pVBuffer = (float*)n_malloc(VBSize);
		pIBuffer = (int*)n_malloc(IBSize);

		pInitMesh->SetUsage(nMesh2::ReadOnly);
		
		// read vertices and indices
		float *pVBuf = pInitMesh->LockVertices();
		memcpy(pVBuffer,pVBuf,VBSize);
		pInitMesh->UnlockVertices();
	    
		ushort *pIBuf = pInitMesh->LockIndices();

		// copy with conversion TriStrip -> TriList
		pIBuffer[0] = pIBuf[0];
		pIBuffer[1] = pIBuf[1];
		pIBuffer[2] = pIBuf[2];
		for(int i = 3, j = 2; i < pInitMesh->GetNumIndices(); i++)
		{
			pIBuffer[++j] = pIBuffer[j - 2];
			pIBuffer[++j] = pIBuffer[j - 2];
			pIBuffer[++j] = pIBuf[i];
		}
		pInitMesh->UnlockIndices();
		*/
	}
	else
	{
		/*
		n_assert(FileName.IsValid());

		if (FileName.CheckExtension("nvx2"))
		{
			nNvx2Loader MeshLoader;
			if (!LoadFromFile(MeshLoader)) FAIL;
		}
		else if (FileName.CheckExtension("n3d2"))
		{
			nN3d2Loader MeshLoader;
			if (!LoadFromFile(MeshLoader)) FAIL;
		}
		else
		{
			n_error("CMeshShape: invalid file extension in '%Sphere'", FileName.CStr());
			FAIL;
		}
		*/
    }

	// fix my collide bits, we don't need to collide against other static and disabled entities
	SetCategoryBits(Static);
	SetCollideBits(Dynamic);

	// create an ODE TriMeshData object from the loaded vertices and indices
	ODETriMeshDataID = dGeomTriMeshDataCreate();

	//!!!CHECK IT!
	// index buffer оказывается должен быть всегда типа int[]
	// хм, эта функция похоже никак не реагирует на передаваемый TriStride,
	// пришлось конвертировать TriStrip в TriList
	dGeomTriMeshDataBuildSingle(ODETriMeshDataID,
								pVBuffer,
								VertexWidth * sizeof(float),
								VertexCount,
								pIBuffer,
								IndexCount,
								3 * sizeof(int));

	ODETriMeshID = dCreateTriMesh(0, ODETriMeshDataID, 0, 0, 0);
	AttachGeom(ODETriMeshID, SpaceID);

	//!!!!!!!FIXME: apply shape mass here!

	OK;
}
//---------------------------------------------------------------------

void CMeshShape::Detach()
{
	n_assert(IsAttached());
	n_assert(pVBuffer);
	n_assert(pIBuffer);

	dGeomTriMeshDataDestroy(ODETriMeshDataID);

	n_free(pVBuffer);
	n_free(pIBuffer);
	pVBuffer = NULL;
	pIBuffer = NULL;

	CShape::Detach();
};
//---------------------------------------------------------------------

/*
// Does a sphere collision check. Returns the number of and the indices
// of all faces inside or intersecting the sphere.
int CMeshShape::DoSphereCollide(const sphere& Sphere, uint*& OutFaceIndices)
{

    Opcode::Model* OPCModel = dGeomTriMeshGetOpcodeModel(ODETriMeshID);
    n_assert(OPCModel);
    IceMaths::Matrix4x4* ModelTfm = (IceMaths::Matrix4x4*)&Transform;

    IceMaths::Sphere OPCSphere(IceMaths::Point(Sphere.p.x, Sphere.p.y, Sphere.p.z), Sphere.r);
    IceMaths::Matrix4x4 SphereTfm;
    SphereTfm.Identity();

    n_assert(OPCSphereCollider.Collide(OPCSphereCache, OPCSphere, *OPCModel, &SphereTfm, ModelTfm));
    int FaceCount = OPCSphereCollider.GetNbTouchedPrimitives();
	OutFaceIndices = (FaceCount > 0) ? (uint*)OPCSphereCollider.GetTouchedPrimitives() : NULL;
    return FaceCount;
}
//---------------------------------------------------------------------

// Do a closest-hit ray check on the shape.
bool CMeshShape::DoRayCheck(const line3& Line, vector3& Contact)
{
	Opcode::Model* OPCModel = dGeomTriMeshGetOpcodeModel(ODETriMeshID);
	n_assert(OPCModel);
	IceMaths::Matrix4x4* ModelTfm = (IceMaths::Matrix4x4*)&Transform;

	static IceMaths::Ray OPCRay;
	OPCRay.mOrig.Set(Line.b.x, Line.b.y, Line.b.z);
	OPCRay.mDir.Set(Line.m.x, Line.m.y, Line.m.z);

	OPCRayCollider.SetMaxDist(Line.m.len());
	OPCRayCollider.Collide(OPCRay, *OPCModel, ModelTfm);
	if (OPCRayCollider.GetContactStatus())
	{
		n_assert(OPCFaces.GetNbFaces() == 1);
		const Opcode::CollisionFace* pFace = OPCFaces.GetFaces();
		Contact = Line.ipol(pFace->mDistance);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------
*/

void CMeshShape::RenderDebug(const matrix44& ParentTfm)
{
	//GFX
	/*
	if (!IsAttached()) return;
	matrix44 Tfm = Transform * ParentTfm;
	nGfxServer2::Instance()->DrawShapeIndexedPrimitives(nGfxServer2::TriangleList,
														IndexCount / 3,
														(vector3*)pVBuffer,
														VertexCount,
														VertexWidth,
														pIBuffer,
														nGfxServer2::Index32,
														Tfm,
														GetDebugVisualizationColor());
	*/
}
//---------------------------------------------------------------------

} // namespace Physics
