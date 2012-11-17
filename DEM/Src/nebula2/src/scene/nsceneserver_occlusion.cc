//------------------------------------------------------------------------------
//  nsceneserver_occlusion.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nrendercontext.h"
#include "scene/nlightnode.h"
#include "gfx2/nocclusionquery.h"

// Issue a general occlusion query for a root scene node group.
void nSceneServer::IssueOcclusionQuery(Group& group, const vector3& viewerPos)
{
    nSceneNode* sceneNode = group.sceneNode;
    n_assert(sceneNode);

    // initialize occlusion flags
    group.renderContext->SetFlag(nRenderContext::Occluded, false);

    // don't do occlusion check for directional light
    if (sceneNode->HasLight() && nLight::Directional == ((nLightNode*)sceneNode)->GetLight().GetType()) return;

    // get conservative bounding boxes in global space, the first for
    // the occlusion check is grown a little to prevent that the object
    // occludes itself, the second checks if the viewer is in the bounding
    // box, and if yes, no occlusion check is done
    const bbox3& globalBox = group.renderContext->GetGlobalBox();

    // check whether the bounding box is very small in one or more dimensions
    // which may lead to z-buffer artifacts during the occlusion check, in that
    // case, don't do an occlusion query for this object
    // FIXME: could also be done once at load time in Mangalore...
    vector3 extents = globalBox.extents();
    if (extents.x < 0.001f || extents.y < 0.001f || extents.z < 0.001f) return;
    
    bbox3 viewerCheckBox(globalBox.center(), globalBox.extents() * 1.2f);
    // check if viewer position is inside current bounding box,
    // if yes, don't perform occlusion check
    if (!viewerCheckBox.contains(viewerPos))
    {
        bbox3 occlusionBox(globalBox.center(), globalBox.extents() * 1.1f);
        // convert back to a matrix for shape rendering
        matrix44 occlusionShapeMatrix = occlusionBox.to_matrix44();
        occlusionQuery->AddShapeQuery(nGfxServer2::Box, occlusionShapeMatrix, &group);
    }
}

//------------------------------------------------------------------------------
/**
    This performs a general occlusion query on all root nodes in the scene.
    Note that light nodes especially benefit from the occlusion query
    since objects which are lit by occluded light sources don't need to
    render the light pass for their lit objects.
*/
void nSceneServer::DoOcclusionQuery()
{
	if (!occlusionQueryEnabled) return;

    PROFILER_START(profOcclusion);

    n_assert(occlusionQuery);

    const vector3& viewerPos = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView).pos_component();
    if (nGfxServer2::Instance()->BeginScene())
    {
        // only update ModelViewProjection matrix in shaders...
        nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, true);

        // issue queries...
        occlusionQuery->Begin();
        for (int i = 0; i < rootArray.Size(); i++)
        {
            Group& group = groupArray[rootArray[i]];
            if (group.renderContext->GetFlag(nRenderContext::DoOcclusionQuery))
				IssueOcclusionQuery(group, viewerPos);
        }
        occlusionQuery->End();

        // get query results...
        // (NOTE: we could split this out and query the results
        // later, since the query will run in parallel to the CPU...
        // if only we had something useful todo in the meantime)
        int numQueries = occlusionQuery->GetNumQueries();
        for (int qi = 0; qi < numQueries; qi++)
            if (occlusionQuery->GetOcclusionStatus(qi))
                ((Group*)occlusionQuery->GetUserData(qi))->renderContext->SetFlag(nRenderContext::Occluded, true);
        occlusionQuery->Clear();

        nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, false);
        nGfxServer2::Instance()->EndScene();
    }

	PROFILER_STOP(profOcclusion);
}
