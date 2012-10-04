//------------------------------------------------------------------------------
//  nswingshapenode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "nature/nswingshapenode.h"
#include "variable/nvariableserver.h"
#include "scene/nrendercontext.h"
#include "gfx2/ngfxserver2.h"

nNebulaClass(nSwingShapeNode, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nSwingShapeNode::nSwingShapeNode() :
    timeVarHandle(nVariable::InvalidHandle),
    windVarHandle(nVariable::InvalidHandle),
    swingAngle(45.0f),
    swingTime(5.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nSwingShapeNode::~nSwingShapeNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination on nMesh2::Usage flags
*/
int
nSwingShapeNode::GetMeshUsage() const
{
    return nMesh2::WriteOnce | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
    This validates the variable handles for time and wind.
*/
bool
nSwingShapeNode::LoadResources()
{
    if (nShapeNode::LoadResources())
    {
        this->timeVarHandle = nVariableServer::Instance()->GetVariableHandleByName("time");
        this->windVarHandle = nVariableServer::Instance()->GetVariableHandleByName("wind");
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    This invalidates the variable handles for time and wind.
*/
void
nSwingShapeNode::UnloadResources()
{
    this->timeVarHandle = nVariable::InvalidHandle;
    this->windVarHandle = nVariable::InvalidHandle;
    nShapeNode::UnloadResources();
}

//------------------------------------------------------------------------------
/**
    Permute the provided static angle by time and world space position. Lots
    of constants here, but the result is quite reasonable for a swinging tree.
    The swinging geometry should not move around in world space, as that
    would break the position offset and lead to stuttering in the
    geometry.

    @param  pos             position in world space
    @param  time            current time
*/
float
nSwingShapeNode::ComputeAngle(const vector3& pos, nTime time) const
{
    // add position offset to time to prevent that all trees swing in sync
    time += pos.x + pos.y + pos.z;

    // sinus wave swing value (between +1 and -1)
    float swing = (float) n_sin((time * n_deg2rad(360.0f)) / this->swingTime);

    // get a wind strength "swinging" angle, we want no swinging at
    // min and max wind strength, and max swinging at 0.5 wind strength
    return this->swingAngle * 0.3f + (this->swingAngle * swing * 0.7f);
}

//------------------------------------------------------------------------------
/**
    Set pre-instancing attribute of shader.
*/
bool
nSwingShapeNode::ApplyShader(nSceneServer* sceneServer)
{
    if (nMaterialNode::ApplyShader(sceneServer))
    {
        nShader2* shader = nGfxServer2::Instance()->GetShader();
        n_assert(shader);

        // set bounding box parameters
        if (shader->IsParameterUsed(nShaderState::BoxMinPos))
        {
            shader->SetVector3(nShaderState::BoxMinPos, this->localBox.vmin);
        }
        if (shader->IsParameterUsed(nShaderState::BoxMaxPos))
        {
            shader->SetVector3(nShaderState::BoxMaxPos, this->localBox.vmax);
        }
        if (shader->IsParameterUsed(nShaderState::BoxCenter))
        {
            shader->SetVector3(nShaderState::BoxCenter, this->localBox.center());
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Set per-instance-attribute of shader.
*/
bool
nSwingShapeNode::RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    if (nMaterialNode::RenderShader(sceneServer, renderContext))
    {
        // get current wind dir and wind strength
        nVariable* timeVar = renderContext->GetVariable(this->timeVarHandle);
        nVariable* windVar = renderContext->GetVariable(this->windVarHandle);
        n_assert(timeVar && windVar);
        nTime time = (nTime)timeVar->GetFloat();
        const nFloat4& wind = windVar->GetFloat4();

        // build horizontal wind vector
        vector3 windVec(wind.x, wind.y, wind.z);

        // get current position in world space
        const matrix44& model = nGfxServer2::Instance()->GetTransform(nGfxServer2::Model);

        // implement swinging by permuting angle by time and position
        float permutedAngle = this->ComputeAngle(model.pos_component(), time);

        // build a rotation matrix from the permuted angle
        static const vector3 up(0.0f, 1.0f, 0.0f);
        matrix44 rotMatrix;
        rotMatrix.rotate(windVec * up, permutedAngle);

        // set shader parameter
        nShader2* shader = nGfxServer2::Instance()->GetShader();
        n_assert(shader);
        if (shader->IsParameterUsed(nShaderState::Swing))
        {
            shader->SetMatrix(nShaderState::Swing, rotMatrix);
        }

        // set wind shader parameter
        if (shader->IsParameterUsed(nShaderState::Wind))
        {
            shader->SetFloat4(nShaderState::Wind, windVar->GetFloat4());
        }
        return true;
    }
    return false;
}
