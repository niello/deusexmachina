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
bool nTransformNode::RenderTransform(nSceneServer* sceneServer,
                                nRenderContext* renderContext,
                                const matrix44& parentMatrix)
{
    n_assert(sceneServer && renderContext);

	// Animate here

    if (GetLockViewer())
    {
        // handle lock to viewer
        const matrix44& viewMatrix = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);
        matrix44 m = tform.getmatrix();
        m = m * parentMatrix;
        m.M41 = viewMatrix.M41;
        m.M42 = viewMatrix.M42;
        m.M43 = viewMatrix.M43;
        sceneServer->SetModelTransform(m);
    }
    else sceneServer->SetModelTransform(tform.getmatrix() * parentMatrix);
    return true;
}
//---------------------------------------------------------------------
