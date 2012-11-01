#include "scene/ntransformnode.h"
#include "scene/nsceneserver.h"
#include "gfx2/ngfxserver2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nTransformNode, "nscenenode");

bool nTransformNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'WVCL': // LCVW
		{
			SetLockViewer(DataReader.Read<bool>());
			OK;
		}
		case 'LCSS': // SSCL
		{
			SetScale(DataReader.Read<vector3>());
			OK;
		}
		case 'TORS': // SROT
		{
			SetQuat(DataReader.Read<quaternion>());
			OK;
		}
		case 'SOPS': // SPOS
		{
			SetPosition(DataReader.Read<vector3>());
			OK;
		}
		case 'TVPR': // RPVT
		{
			SetRotatePivot(DataReader.Read<vector3>());
			OK;
		}
		case 'TVPS': // SPVT
		{
			SetScalePivot(DataReader.Read<vector3>());
			OK;
		}
		default: return nSceneNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

void nTransformNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
	n_assert(sceneServer);
	if (CheckFlags(Active))
	{
		sceneServer->BeginGroup(this, renderContext);
		nSceneNode::Attach(sceneServer, renderContext);
		sceneServer->EndGroup();
	}
}
//---------------------------------------------------------------------

// Compute the resulting modelview matrix and set it in the scene server as current modelview matrix.
bool nTransformNode::RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix)
{
	n_assert(sceneServer && renderContext);

	// Animate here

	//!!!can optimize - write result to WorldTfm! (binary op or smth)
	matrix44 WorldTfm = tform.getmatrix();
	WorldTfm.mult_simple(parentMatrix);

	if (GetLockViewer())
		WorldTfm.set_translation(nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView).pos_component());

	sceneServer->SetModelTransform(WorldTfm);

	return true;
}
//---------------------------------------------------------------------
