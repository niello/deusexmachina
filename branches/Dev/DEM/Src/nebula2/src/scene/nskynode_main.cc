//------------------------------------------------------------------------------
//  nskynode_main.cc
//  (C) 2005 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nskynode.h"
#include "gfx2/ngfxserver2.h"
#include "mathlib/polar.h"
#include "variable/nvariableserver.h"
#include "scene/nrendercontext.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSkyNode, "ntransformnode");

const float nSkyNode::CloudSpeedFactor = 0.0005f;

//------------------------------------------------------------------------------
/**
*/
nSkyNode::nSkyNode() :
    timeFactor(24),
    timePeriod(86400),
    startTime(0),
    sunSpeedFactor(360.0f/this->timePeriod),
    skyTime(0),
    worldTime(0),
    jumpTime(0)
{
    this->SetLockViewer(true);
    this->SetChannel("time");
    //this->SetSaveModeFlags(nRoot::N_FLAG_SAVEUPSIDEDOWN);
}

bool nSkyNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'FMIT': // TIMF
		{
			SetTimeFactor(DataReader.Read<float>());
			OK;
		}
		case 'PMIT': // TIMP
		{
			SetTimePeriod(DataReader.Read<float>());
			OK;
		}
		case 'MITS': // STIM
		{
			SetStartTime(DataReader.Read<float>());
			OK;
		}
		case 'LEKS': // SKEL
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				char Value[512];
				if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
				nSkyNode::ElementType Type = StringToType(Value);
				if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
				//AddElement(Type, Value);
			}
			OK;
		}
		case 'LYKS': // SKYL
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			n_assert(!(Count % 2));
			for (short i = 0; i < Count; i += 2) // Pairs From -> To
			{
				char From[128];
				if (!DataReader.ReadString(From, sizeof(From))) FAIL;
				char To[128];
				if (!DataReader.ReadString(To, sizeof(To))) FAIL;
				//LinkTo(From, To);
			}
			OK;
		}
		default: return nTransformNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Attach to the scene server.
*/
void
nSkyNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->HChannel);
    // get the sample time from the render context
    nVariable* var = renderContext->GetVariable(this->HChannel);
    n_assert(var);
    float time = var->GetFloat();

    this->UpdateSky(time);
    renderContext->SetFlag(nRenderContext::CastShadows, false); // added by John Smith

    nTransformNode::Attach(sceneServer, renderContext);
}

//------------------------------------------------------------------------------
/**
    Adds an existing Object as an element
*/
void
nSkyNode::AddElement(nSkyNode::ElementType type, const nString& name)
{
    nKernelServer::Instance()->PushCwd(this);
    nRoot* element = nKernelServer::Instance()->Lookup(name.Get());
    nKernelServer::Instance()->PopCwd();
    n_assert(element);

    ElementGroup newElement;
    newElement.type = type;
    newElement.states.Clear();
    newElement.lastRefresh = this->skyTime;
    newElement.refreshTime = 0.0f;

    switch (type)
    {
    case nSkyNode::SkyElement:
        {
            n_assert(element->IsInstanceOf("nshapenode"));
            nShapeNode* newNode = (nShapeNode*)element;

            if (newNode->GetMesh().IsEmpty())
            {
                newNode->SetMesh("meshes:examples/skyElements_s_0.n3d2");
                newNode->SetGroupIndex(0);
            }
            if (newNode->GetShader().IsEmpty())
            {
                newNode->SetShader("sky");
            }
            newNode->SetLockViewer(true);
            if (!newNode->HasParam(nShaderState::Intensity0))
                newNode->SetFloat(nShaderState::Intensity0, 1.0f);
            if (!newNode->HasParam(nShaderState::SunFlat))
                newNode->SetFloat(nShaderState::SunFlat, 0.0f);
            if (!newNode->HasParam(nShaderState::SunRange))
                newNode->SetFloat(nShaderState::SunRange, 1.0f);
            if (!newNode->HasParam(nShaderState::SkyBottom))
                newNode->SetFloat(nShaderState::SkyBottom, 0.0f);
            if (!newNode->HasParam(nShaderState::Saturation))
                newNode->SetVector(nShaderState::Saturation, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::Brightness))
                newNode->SetVector(nShaderState::Brightness, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::TopColor))
                newNode->SetVector(nShaderState::TopColor, vector4(0.1f, 0.1f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::BottomColor))
                newNode->SetVector(nShaderState::BottomColor, vector4(0.6f, 0.6f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::SunColor))
                newNode->SetVector(nShaderState::SunColor, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            break;
        }
    case nSkyNode::SunElement:
        {
            n_assert(element->IsInstanceOf("nshapenode"));
            nShapeNode* newNode = (nShapeNode*)element;

            if (newNode->GetMesh().IsEmpty())
            {
                newNode->SetMesh("meshes:examples/skyElements_s_0.n3d2");
                newNode->SetGroupIndex(3);
            }
            if (!newNode->GetTexture(nShaderState::DiffMap0))
                newNode->SetTexture(nShaderState::DiffMap0, "textures:environment/sun.dds");
            if (newNode->GetShader().IsEmpty())
            {
                newNode->SetShader("sun");
            }
            newNode->SetLockViewer(false);
            if (!newNode->HasParam(nShaderState::Intensity0))
                newNode->SetFloat(nShaderState::Intensity0, 1.0f);
            if (!newNode->HasParam(nShaderState::MatDiffuse))
                newNode->SetVector(nShaderState::MatDiffuse, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::Move))
                newNode->SetVector(nShaderState::Move, vector4(0.0f, 1.0f, 0.0f, 0.0f));
            if (!newNode->HasParam(nShaderState::ScaleVector))
                newNode->SetVector(nShaderState::ScaleVector, vector4(1.0f, 1.0f, 1.0f, 1.5f));
            if (!newNode->HasParam(nShaderState::Position))
                newNode->SetVector(nShaderState::Position, vector4(40.0f, -90.0f, 20.0f, 0.0f));
            else
            {
                vector4 vec = newNode->GetVector(nShaderState::Position);
                vec.y = -90.0f;
                newNode->SetVector(nShaderState::Position, vec);
            }

            if (!newNode->HasParam(nShaderState::LightDiffuse))
                newNode->SetVector(nShaderState::LightDiffuse, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightDiffuse1))
                newNode->SetVector(nShaderState::LightDiffuse1, vector4(0.1f, 0.1f, 0.15f, 1.0f));
            break;
        }
    case nSkyNode::LightElement:
        {
            n_assert(element->IsInstanceOf("nlightnode"));
            nLightNode* newNode = (nLightNode*)element;

            newNode->SetInt(nShaderState::LightType, nLight::Directional);
            if (!newNode->HasParam(nShaderState::LightDiffuse))
                newNode->SetVector(nShaderState::LightDiffuse, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightSpecular))
                newNode->SetVector(nShaderState::LightSpecular, vector4(0.5f, 0.5f, 0.5f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightAmbient))
                newNode->SetVector(nShaderState::LightAmbient, vector4(0.1f, 0.1f, 0.15f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightDiffuse1))
                newNode->SetVector(nShaderState::LightDiffuse1, vector4(0.3f, 0.3f, 0.3f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightRange))
                newNode->SetFloat(nShaderState::LightRange, 5000.0f);
            newNode->SetLocalBox(bbox3(vector3::Zero, vector3(5000.0f, 5000.0f, 5000.0f)));
            break;
        }
    case nSkyNode::CloudElement:
        {
            n_assert(element->IsInstanceOf("nshapenode"));
            nShapeNode* newNode = (nShapeNode*)element;

            if (newNode->GetMesh().IsEmpty())
            {
                newNode->SetMesh("meshes:examples/skyElements_s_0.n3d2");
                newNode->SetGroupIndex(2);
            }
            if (newNode->GetShader().IsEmpty())
            {
                newNode->SetShader("cloud");
            }
            newNode->SetLockViewer(true);
            if (!newNode->GetTexture(nShaderState::BumpMap0))
                newNode->SetTexture(nShaderState::BumpMap0, "textures:environment/cloud_bump_ripple.dds");
            if (!newNode->GetTexture(nShaderState::DiffMap0))
                newNode->SetTexture(nShaderState::DiffMap0, "textures:environment/cloud_map_cloudy.dds");
            if (!newNode->GetTexture(nShaderState::DiffMap1))
                newNode->SetTexture(nShaderState::DiffMap1, "textures:environment/cloud_map_cloudy1.dds");
            newNode->SetInt(nShaderState::CullMode, 1);
            if (!newNode->HasParam(nShaderState::Map0uvRes))
                newNode->SetFloat(nShaderState::Map0uvRes, 1.0f);
            if (!newNode->HasParam(nShaderState::Map1uvRes))
                newNode->SetFloat(nShaderState::Map1uvRes, 1.0f);
            if (!newNode->HasParam(nShaderState::MatDiffuse))
                newNode->SetVector(nShaderState::MatDiffuse, vector4(0.9f,0.9f,1.1f,0.7f));
            if (!newNode->HasParam(nShaderState::Move))
                newNode->SetVector(nShaderState::Move, vector4(0.2f, 0.1f, 0.15f, 0.08f));
            if (!newNode->HasParam(nShaderState::CloudMod))
                newNode->SetVector(nShaderState::CloudMod, vector4(0.0f, 1.0f, 0.0f, 0.0f));
            if (!newNode->HasParam(nShaderState::CloudPos))
                newNode->SetVector(nShaderState::CloudPos, vector4(0.0f, 0.0f, 0.0f, 0.0f));
            if (!newNode->HasParam(nShaderState::CloudGrad))
                newNode->SetVector(nShaderState::CloudGrad, vector4(0.0f, 3.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightDiffuse))
                newNode->SetVector(nShaderState::LightDiffuse, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::LightDiffuse1))
                newNode->SetVector(nShaderState::LightDiffuse1, vector4(0.1f, 0.1f, 0.15f, 1.0f));
            if (!newNode->HasParam(nShaderState::SunRange))
                newNode->SetFloat(nShaderState::SunRange, 1.0f);
            if (!newNode->HasParam(nShaderState::Density))
                newNode->SetFloat(nShaderState::Density, 0.0f);
            if (!newNode->HasParam(nShaderState::Lightness))
                newNode->SetFloat(nShaderState::Lightness, 0.2f);
            if (!newNode->HasParam(nShaderState::Map0uvRes))
                newNode->SetFloat(nShaderState::Map0uvRes, 1.0f);
            if (!newNode->HasParam(nShaderState::Map0uvRes))
                newNode->SetFloat(nShaderState::Map0uvRes, 1.2f);
            if (!newNode->HasParam(nShaderState::Glow))
                newNode->SetFloat(nShaderState::Glow, 0.5f);
            if (!newNode->HasParam(nShaderState::Position))
                newNode->SetVector(nShaderState::Position, vector4(1.0f, 3.0f, 1.0f, 0.0f));
            if (!newNode->HasParam(nShaderState::Weight))
                newNode->SetFloat(nShaderState::Weight, 0.7f);
            if (!newNode->HasParam(nShaderState::BumpFactor))
                newNode->SetFloat(nShaderState::BumpFactor, 4.0f);
            break;
        }
    case nSkyNode::StarElement:
        {
            n_assert(element->IsInstanceOf("nshapenode"));
            nShapeNode* newNode = (nShapeNode*)element;

            if (newNode->GetMesh().IsEmpty())
            {
                newNode->SetMesh("meshes:examples/skyElements_s_0.n3d2");
                newNode->SetGroupIndex(1);
            }
            if (newNode->GetShader().IsEmpty())
            {
                newNode->SetShader("stars");
            }
            if (!newNode->GetTexture(nShaderState::DiffMap0))
                newNode->SetTexture(nShaderState::DiffMap0, "textures:environment/stars_tile.dds");
            if (!newNode->GetTexture(nShaderState::DiffMap1))
                newNode->SetTexture(nShaderState::DiffMap1, "textures:environment/stars_spread.dds");
            if (!newNode->HasParam(nShaderState::Intensity0))
                newNode->SetFloat(nShaderState::Intensity0, 1.0f);
            if (!newNode->HasParam(nShaderState::Saturation))
                newNode->SetVector(nShaderState::Saturation, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            if (!newNode->HasParam(nShaderState::Brightness))
                newNode->SetVector(nShaderState::Brightness, vector4(1.0f, 1.0f, 1.0f, 1.0f));
            break;
        }
    default:
        n_assert(element->IsA("nabstractshadernode"));
        newElement.type = nSkyNode::GenericElement;
        break;
    }
    newElement.refElement.set((nAbstractShaderNode*)element);
    this->elements.Append(newElement);
}

//------------------------------------------------------------------------------
/**
    Attaches a state to a sky element and defines a point of time,
    when it should be applied
*/
int
nSkyNode::AddState(const nString& destName, const nString& stateName, float time)
{
    int j;
    bool addstate_ok = false;
    for (int i = 0; i < this->elements.Size(); i++)
    {
        if (destName == nString(this->elements[i].refElement->GetName()))
        {
            nKernelServer::Instance()->PushCwd(this);
            nSkyState* state = (nSkyState*)nKernelServer::Instance()->Lookup(stateName.Get());
            nKernelServer::Instance()->PopCwd();
            if (state)
            {
                StateGroup newState;
                newState.refState.set(state);
                newState.time = time;
                for (j = 0; j < this->elements[i].states.Size(); j++)
                {
                    if (time < this->elements[i].states[j].time)
                    {
                        this->elements[i].states.Insert(j, newState);
                        addstate_ok = true;
                        break;
                    }
                }
                if (!addstate_ok)
                {
                    this->elements[i].states.Append(newState);
                    addstate_ok = true;
                }
            }
            break;
        }
    }
    n_assert(addstate_ok);
    return j;
}

//------------------------------------------------------------------------------
/**
    Removes a state from an element
*/
void
nSkyNode::RemoveState(const nString& destName, const nString& stateName)
{
    int element = -1;
    for (int iEl = 0; iEl < this->elements.Size(); iEl++)
    {
        if (destName == this->elements[iEl].refElement->GetName())
        {
            element = iEl;
            break;
        }
    }
    n_assert(element != -1);

    int state = -1;
    for (int iSt = 0; iSt < this->elements[element].states.Size(); iSt++)
    {
        if (stateName == this->elements[element].states[iSt].refState->GetName())
        {
            state = iSt;
            break;
        }
    }
    n_assert(state != -1);
    this->elements[element].states.Erase(state);
}

//------------------------------------------------------------------------------
/**
    Creates a new sky element and initializes it.
*/
void
nSkyNode::NewElement(nSkyNode::ElementType type, const nString& name)
{

   switch (type)
   {
   case nSkyNode::SkyElement:
   case nSkyNode::SunElement:
   case nSkyNode::CloudElement:
   case nSkyNode::StarElement:
       nKernelServer::Instance()->PushCwd(this);
       nKernelServer::Instance()->New("nshapenode", name.Get());
       nKernelServer::Instance()->PopCwd();
       this->AddElement(type, name);
       break;
   case nSkyNode::LightElement:
       nKernelServer::Instance()->PushCwd(this);
       nKernelServer::Instance()->New("nlightnode", name.Get());
       nKernelServer::Instance()->PopCwd();
       this->AddElement(type, name);
       break;
   case nSkyNode::GenericElement:
       nKernelServer::Instance()->PushCwd(this);
       nKernelServer::Instance()->New("nshapenode", name.Get());
       nKernelServer::Instance()->PopCwd();
       this->AddElement(nSkyNode::GenericElement, name);
       break;
   }
}

//------------------------------------------------------------------------------
/**
    Deletes an existing sky element
*/
void
nSkyNode::DeleteElement(const nString &name)
{
    int element = -1;
    for (int iEl = 0; iEl < this->elements.Size(); iEl++)
    {
        if (name == this->elements[iEl].refElement->GetName())
        {
            element = iEl;
            break;
        }
    }

    n_assert(element != -1);
    //this->elements[element].refElement.Remove(); //FIXME: Delete or just remove?
    this->elements.Erase(element);
}

//------------------------------------------------------------------------------
/**
    links one element to another
*/
void
nSkyNode::LinkTo(const nString& fromName, const nString& toName)
{
    this->elements[this->FindElement(fromName)].linkTo.Append(this->FindElement(toName));
}

//------------------------------------------------------------------------------
/**
    returns the index of an element
*/
int
nSkyNode::FindElement(const nString& name)
{
    for (int i = 0; i < this->elements.Size() ; i++)
    {
        if (name == elements[i].refElement->GetName()) return i;
    }
    return -1;
}

//------------------------------------------------------------------------------
/**
    returns the index of an state
*/
int
nSkyNode::FindState(int elementNr, const nString& stateName)
{
    for (int i = 0; i < this->elements[elementNr].states.Size() ; i++)
    {
        if (stateName == elements[elementNr].states[i].refState->GetName()) return i;
    }
    return -1;
}

//------------------------------------------------------------------------------
/**

*/
void
nSkyNode::FindStates(int element, float time, int& state0, int& state1)
{
    state0 = this->elements[element].states.Size() - 1;
    state1 = 0;
    for (int iSt = 0; iSt < this->elements[element].states.Size(); iSt++)
    {
        if (this->elements[element].states[iSt].time <= time)
        {
            if (iSt == (this->elements[element].states.Size() - 1))
            {
                state0 = iSt;
                state1 = 0;
                break;
            }
            else if (this->elements[element].states[iSt+1].time > time)
            {
                state0 = iSt;
                state1 = iSt + 1;
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**

*/
void
nSkyNode::SortOutParams(int element, ParamList& paramList)
{
    switch (this->elements[element].type)
    {
    case nSkyNode::SkyElement:
    case nSkyNode::LightElement:
    case nSkyNode::GenericElement:
        break;
    case nSkyNode::SunElement:
        {
            int i = paramList.vectorParams.FindIndex(nShaderState::Position);
            if (i != -1) paramList.vectorParams.Set(i, nShaderState::InvalidParameter);
        }
        break;
    case nSkyNode::CloudElement:
        {
            int i = paramList.vectorParams.FindIndex(nShaderState::Move);
            if (i != -1) paramList.vectorParams.Set(i, nShaderState::InvalidParameter);
        }
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**

*/
float
nSkyNode::ComputeWeight(int element, float time, int state0, int state1)
{
    float stateTime0 = this->elements[element].states[state0].time;
    float stateTime1 = this->elements[element].states[state1].time;
    if (stateTime0 > stateTime1) stateTime1 += this->timePeriod;    //carry from previous period;
    float statePeriode = stateTime1 - stateTime0;
    float relTime = time - stateTime0;
    if (relTime < 0) relTime += this->timePeriod;

    return (relTime / statePeriode);
}

//------------------------------------------------------------------------------
/**
    Updates all elements of the nskynode
*/
void
nSkyNode::UpdateSky(float newTime)
{
    float time = newTime - this->worldTime;
    this->worldTime = newTime;
    time *= this->timeFactor;
    time += this->skyTime + this->jumpTime;
    while (time < 0) time += this->timePeriod;
    time = (float)fmod(time,this->timePeriod);
    this->skyTime = time;

    vector3 cameraPos = this->GetCameraPos();

    // update each element
    for (int iEl = 0; iEl < this->elements.Size(); iEl++)
    {
        // time exception
        if (this->elements[iEl].lastRefresh > this->worldTime) this->elements[iEl].lastRefresh = this->worldTime - this->elements[iEl].refreshTime;
        // only update element if refresh time elapsed
        // never update element if refresh time is set to -1
        if ((this->elements[iEl].refreshTime != -1) &&
        ((this->elements[iEl].lastRefresh + this->elements[iEl].refreshTime) <= this->worldTime))
        {
            // only update element, if it owns states
            if (this->elements[iEl].states.Size() > 0)
            {
                // Find relevant states
                int state0;
                int state1;
                this->FindStates(iEl, time, state0, state1);

                // compute stateWeight
                float stateWeight = this->ComputeWeight(iEl, time, state0, state1);

                // Collect all params of both states
                ParamList paramList;
                paramList.floatParams.Clear();
                paramList.intParams.Clear();
                paramList.vectorParams.Clear();
                this->CollectParams(this->elements[iEl].states[state0].refState->GetShaderParams(), paramList);
                this->CollectParams(this->elements[iEl].states[state1].refState->GetShaderParams(), paramList);

                // sort out params, not to apply generic
                this->SortOutParams(iEl, paramList);

                vector4 vec0;
                vector4 vec1;
                float flt0;
                float flt1;
                int int0;
                int int1;

                // apply state vector params to element
                for (int iPa = 0; iPa < paramList.vectorParams.Size(); iPa++)
                {
                    if (nShaderState::InvalidParameter != paramList.vectorParams[iPa])
                    {
                        if (this->elements[iEl].states[state0].refState->HasParam(paramList.vectorParams[iPa]))
                            vec0 = this->elements[iEl].states[state0].refState->GetVector(paramList.vectorParams[iPa]);
                        else vec0 = this->elements[iEl].states[state1].refState->GetVector(paramList.vectorParams[iPa]);

                        if (this->elements[iEl].states[state1].refState->HasParam(paramList.vectorParams[iPa]))
                            vec1 = this->elements[iEl].states[state1].refState->GetVector(paramList.vectorParams[iPa]);
                        else vec1 = this->elements[iEl].states[state0].refState->GetVector(paramList.vectorParams[iPa]);

                        this->elements[iEl].refElement->SetVector(paramList.vectorParams[iPa],  vec0*(1-stateWeight) + vec1*stateWeight);
                    }
                }

                // apply state float params to element
                for (int iPa = 0; iPa < paramList.floatParams.Size(); iPa++)
                {
                    if (nShaderState::InvalidParameter != paramList.floatParams[iPa])
                    {
                        if (this->elements[iEl].states[state0].refState->HasParam(paramList.floatParams[iPa]))
                            flt0 = this->elements[iEl].states[state0].refState->GetFloat(paramList.floatParams[iPa]);
                        else flt0 = this->elements[iEl].states[state1].refState->GetFloat(paramList.floatParams[iPa]);

                        if (this->elements[iEl].states[state1].refState->HasParam(paramList.floatParams[iPa]))
                            flt1 = this->elements[iEl].states[state1].refState->GetFloat(paramList.floatParams[iPa]);
                        else flt1 = this->elements[iEl].states[state0].refState->GetFloat(paramList.floatParams[iPa]);

                        this->elements[iEl].refElement->SetFloat(paramList.floatParams[iPa],  flt0*(1-stateWeight) + flt1*stateWeight);
                    }
                }

                // apply state int params to element
                for (int iPa = 0; iPa < paramList.intParams.Size(); iPa++)
                {
                    if (nShaderState::InvalidParameter != paramList.intParams[iPa])
                    {
                        if (this->elements[iEl].states[state0].refState->HasParam(paramList.intParams[iPa]))
                            int0 = this->elements[iEl].states[state0].refState->GetInt(paramList.intParams[iPa]);
                        else int0 = this->elements[iEl].states[state1].refState->GetInt(paramList.intParams[iPa]);

                        if (this->elements[iEl].states[state1].refState->HasParam(paramList.intParams[iPa]))
                            int1 = this->elements[iEl].states[state1].refState->GetInt(paramList.intParams[iPa]);
                        else int1 = this->elements[iEl].states[state0].refState->GetInt(paramList.intParams[iPa]);

                        this->elements[iEl].refElement->SetInt(paramList.intParams[iPa],  n_frnd(int0*(1-stateWeight) + int1*stateWeight));
                    }
                }

                // apply special attributes and methods
                switch (this->elements[iEl].type)
                {
                case nSkyNode::SkyElement:
                    //apply linked elements
                    for (int iLk = 0; iLk < this->elements[iEl].linkTo.Size(); iLk++)
                    {
                        if (this->elements[iEl].linkTo[iLk] != -1)
                        {
                            switch (this->elements[this->elements[iEl].linkTo[iLk]].type)
                            {
                            case nSkyNode::SunElement:
                                {
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetPosition();
                                    this->elements[iEl].refElement->SetVector(nShaderState::Position, vec0);

                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetVector(nShaderState::MatDiffuse);
                                    vec0.w = 1;
                                    vec0.saturate();
                                    this->elements[iEl].refElement->SetVector(nShaderState::SunColor, vec0);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                case nSkyNode::SunElement:
                    // compute position data
                    if (this->elements[iEl].states[state0].refState->HasParam(nShaderState::Position))
                        vec0 = this->elements[iEl].states[state0].refState->GetVector(nShaderState::Position);
                    else vec0 = this->elements[iEl].refElement->GetVector(nShaderState::Position);
                    if (this->elements[iEl].states[state1].refState->HasParam(nShaderState::Position))
                        vec1 = this->elements[iEl].states[state1].refState->GetVector(nShaderState::Position);
                    else vec1 = this->elements[iEl].refElement->GetVector(nShaderState::Position);
                    if (vec0.w >= 0.0f)    // sun rotates from east to west
                    {
                        if (vec0.x > vec1.x) vec1.x += 360.0f;
                        if (vec0.y > vec1.y) vec1.y += 360.0f;
                    }
                    else                // sun rotates from west to east
                    {
                        if (vec1.x > vec0.x) vec0.x += 360.0f;
                        if (vec1.y > vec0.y) vec0.y += 360.0f;
                    }
                    vec0 = vec0 * (1-stateWeight) + vec1 * stateWeight;
                    this->elements[iEl].refElement->SetVector(nShaderState::Position, vec0);
                    // compute sun movement
                    vec0 = this->elements[iEl].refElement->GetVector(nShaderState::Move);
                    vec1 = this->elements[iEl].refElement->GetVector(nShaderState::Position);
                    vec1 += vec0 * this->sunSpeedFactor * this->jumpTime;
                    vec0 = vec1 + (vec0 * this->sunSpeedFactor * this->timeFactor * (this->worldTime - this->elements[iEl].lastRefresh));
                    vec0.x = (float)fmod(vec0.x, 360.0f);
                    vec0.y = (float)fmod(vec0.y, 360.0f);
                    this->elements[iEl].refElement->SetVector(nShaderState::Position, vec0);
                    // apply position
                    vec0 = this->elements[iEl].refElement->GetVector(nShaderState::Position);
                    this->elements[iEl].refElement->SetPosition(this->GetSphericalCoordinates(vector3(vec0.x, vec0.y, vec0.z)));
                    this->SetFaceToViewer(iEl);

                    // apply scale data
                    vec0 = this->elements[iEl].refElement->GetVector(nShaderState::ScaleVector);
                    this->elements[iEl].refElement->SetScale(vector3(vec0.x * vec0.w, vec0.y * vec0.w, 1.0f));
                    break;
                case nSkyNode::LightElement:
                    {
                        bool posSet = false;
                        //apply linked elements
                        for (int iLk = 0; iLk < this->elements[iEl].linkTo.Size(); iLk++)
                        {
                            if (this->elements[iEl].linkTo[iLk] != -1)
                            {
                                switch (this->elements[this->elements[iEl].linkTo[iLk]].type)
                                {
                                case nSkyNode::SunElement:
                                    {
                                        vector3 pos = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetPosition() * 1000;
                                        this->elements[iEl].refElement->SetPosition( pos );
                                        posSet = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (!posSet)   // get pos from states, if not set yet
                        {
                            bool paramsOk = true;
                            if (this->elements[iEl].states[state0].refState->HasParam(nShaderState::Position))
                                vec0 = this->elements[iEl].states[state0].refState->GetVector(nShaderState::Position);
                            else paramsOk = false;
                            if (this->elements[iEl].states[state1].refState->HasParam(nShaderState::Position))
                                vec1 = this->elements[iEl].states[state1].refState->GetVector(nShaderState::Position);
                            else paramsOk = false;
                            if (paramsOk)
                            {
                                vec0 = vec0 * (1-stateWeight) + vec1 * stateWeight;
                                this->elements[iEl].refElement->SetPosition(this->GetSphericalCoordinates(vector3(vec0.x, vec0.y, vec0.z)));
                            }
                        }
                    }
                    break;
                case nSkyNode::CloudElement:
                    {
                        // compute cloud move
                        if (this->elements[iEl].states[state0].refState->HasParam(nShaderState::Move))
                            vec0 = this->elements[iEl].states[state0].refState->GetVector(nShaderState::Move);
                        else vec0 = this->elements[iEl].refElement->GetVector(nShaderState::Move);

                        if (this->elements[iEl].states[state1].refState->HasParam(nShaderState::Move))
                            vec1 = this->elements[iEl].states[state1].refState->GetVector(nShaderState::Move);
                        else vec1 = this->elements[iEl].refElement->GetVector(nShaderState::Move);

                        vec0 = vec0 * (1-stateWeight) + vec1 * stateWeight;
                        vec1 = this->elements[iEl].refElement->GetVector(nShaderState::CloudPos);
                        vec1 += vec0 * this->CloudSpeedFactor * this->jumpTime;
                        vec0 = vec1 + (vec0 * this->CloudSpeedFactor * this->timeFactor * (this->worldTime - this->elements[iEl].lastRefresh));

                        vec0.x = (float)fmod(vec0.x,1.0f);
                        vec0.y = (float)fmod(vec0.y,1.0f);
                        vec0.z = (float)fmod(vec0.z,1.0f);
                        vec0.w = (float)fmod(vec0.w,1.0f);

                        this->elements[iEl].refElement->SetVector(nShaderState::CloudPos, vec0);

                        //apply linked elements
                        for (int iLk = 0; iLk < this->elements[iEl].linkTo.Size(); iLk++)
                        {
                            if (this->elements[iEl].linkTo[iLk] != -1)
                            {
                                switch (this->elements[this->elements[iEl].linkTo[iLk]].type)
                                {
                                case nSkyNode::SunElement:                 //FIXME: Test fehlt, ob LightDiffuse gesetzt wurde!
                                    //get sun position
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetPosition();
                                    this->elements[iEl].refElement->SetVector(nShaderState::Position, vec0 - cameraPos);
                                    //get sun light color
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetVector(nShaderState::LightDiffuse);
                                    this->elements[iEl].refElement->SetVector(nShaderState::LightDiffuse, vec0);
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetVector(nShaderState::LightDiffuse1);
                                    this->elements[iEl].refElement->SetVector(nShaderState::LightDiffuse1, vec0);
                                    break;
                                case nSkyNode::LightElement:
                                    //get sun position
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetPosition();
                                    this->elements[iEl].refElement->SetVector(nShaderState::Position, vec0 - cameraPos);
                                    //get sun light color
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetVector(nShaderState::LightDiffuse);
                                    this->elements[iEl].refElement->SetVector(nShaderState::LightDiffuse, vec0);
                                    vec0 = this->elements[this->elements[iEl].linkTo[iLk]].refElement->GetVector(nShaderState::LightDiffuse1);
                                    this->elements[iEl].refElement->SetVector(nShaderState::LightDiffuse1, vec0);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            // remember last refresh
            this->elements[iEl].lastRefresh = this->worldTime;
        }
    }
    this->jumpTime = 0;
}

//------------------------------------------------------------------------------
/**
    Updates the angle of the element, to ensure being faced to viewer
*/
void
nSkyNode::SetFaceToViewer(int element)
{
    polar2 viewerAngles(-this->elements[element].refElement->GetPosition());
    viewerAngles.theta -= N_PI*0.5f;
    matrix44 m;
    m.rotate_x(viewerAngles.theta);
    m.rotate_y(viewerAngles.rho);
    this->elements[element].refElement->SetQuat(m.get_quaternion());
}

//------------------------------------------------------------------------------
/**
    Returns cartesian coordinates from spherical coordinates
*/
vector3
nSkyNode::GetSphericalCoordinates(vector3 angles)
{
    angles.x += 90.0f;
    angles *= N_PI/180.0f;
    vector3 pos;
    pos.x = n_sin(angles.x) * n_cos(angles.y);
    pos.y = n_sin(angles.x) * n_sin(angles.y);
    pos.z = n_cos(angles.x);
    pos.rotate(vector3(1.0f, 0.0f, 0.0f), angles.z);
    return pos;
}

//------------------------------------------------------------------------------
/**
    Sets the refresh time for an element
*/
void
nSkyNode::SetRefreshTime(const nString& name, float time)
{
    this->elements[this->FindElement(name)].refreshTime = time;
}

//------------------------------------------------------------------------------
/**
    Gets the refresh time of an element
*/
float
nSkyNode::GetRefreshTime(const nString& name)
{
    return this->elements[this->FindElement(name)].refreshTime;
}

//------------------------------------------------------------------------------
/**

*/
int
nSkyNode::SetStateTime(const nString& elementName, const nString& stateName, float time)
{
    int elementNr = this->FindElement(elementName);
    int stateNr = this->FindState(elementNr, stateName);
    return this->SetStateTime(elementNr, stateNr, time);
}

//------------------------------------------------------------------------------
/**

*/
int
nSkyNode::SetStateTime(int elementNr,int stateNr, float time)
{
    int state0;
    int state1;
    this->FindStates(elementNr, time, state0, state1);
    if (state0 == stateNr || state1 == stateNr)
    {
        this->elements[elementNr].states[stateNr].time = time;
        return stateNr;
    }
    else
    {
        nString stateName = this->elements[elementNr].states[stateNr].refState->GetName();
        nString elementName = this->elements[elementNr].refElement->GetName();
        this->RemoveState(elementName, stateName);
        return this->AddState(elementName, stateName, time);
    }
}

//------------------------------------------------------------------------------
/**

*/
float
nSkyNode::GetStateTime(const nString& elementName, const nString& stateName)
{
    int elementNr = this->FindElement(elementName);
    int stateNr = this->FindState(elementNr, stateName);
    return this->GetStateTime(elementNr, stateNr);
}

//------------------------------------------------------------------------------
/**

*/
float
nSkyNode::GetStateTime(int elementNr,int stateNr)
{
    return this->elements[elementNr].states[stateNr].time;
}

//------------------------------------------------------------------------------
/**
    Gets number of elements
*/
int
nSkyNode::GetNumElements()
{
    return this->elements.Size();
}

//------------------------------------------------------------------------------
/**
    Gets number of states of an element
*/
int
nSkyNode::GetNumStates(int elementNr)
{
    return this->elements[elementNr].states.Size();
}

//------------------------------------------------------------------------------
/**
    Gets a pointer to an element (by name)
*/
nAbstractShaderNode*
nSkyNode::GetElement(const nString& name)
{
    return this->elements[this->FindElement(name)].refElement;
}

//------------------------------------------------------------------------------
/**
    Gets a pointer to an element (by number)
*/
nAbstractShaderNode*
nSkyNode::GetElement(int elementNr)
{
    return this->elements[elementNr].refElement;
}

//------------------------------------------------------------------------------
/**
    Gets a pointer to an state of an element (by name)
*/
nSkyState*
nSkyNode::GetState(const nString& elementName, const nString& stateName)
{
    return this->elements[this->FindElement(elementName)].states[this->FindElement(stateName)].refState;
}

//------------------------------------------------------------------------------
/**
    Gets a pointer to an state of an element (by number)
*/
nSkyState*
nSkyNode::GetState(int elementNr, int stateNr)
{
    return this->elements[elementNr].states[stateNr].refState;
}

//------------------------------------------------------------------------------
/**
    Get element type (by name)
*/
nSkyNode::ElementType
nSkyNode::GetElementType(const nString& name)
{
    return this->elements[this->FindElement(name)].type;
}

//------------------------------------------------------------------------------
/**
    Get element type (by number)
*/
nSkyNode::ElementType
nSkyNode::GetElementType(int elementNr)
{
    return this->elements[elementNr].type;
}

//------------------------------------------------------------------------------
/**
    Adds all params from the nShaderParams to the paramList
*/
void
nSkyNode::CollectParams(nShaderParams params, ParamList& paramList)
{
    for (int i = 0; i < params.GetNumValidParams(); i++)
    {
        switch (params.GetArgByIndex(i).GetType())
        {
        case nShaderState::Float4:
            if (paramList.vectorParams.FindIndex(params.GetParamByIndex(i)) == -1)
                paramList.vectorParams.Append(params.GetParamByIndex(i));
            break;
        case nShaderState::Int:
            if (paramList.intParams.FindIndex(params.GetParamByIndex(i)) == -1)
                paramList.intParams.Append(params.GetParamByIndex(i));
            break;
        case nShaderState::Float:
            if (paramList.floatParams.FindIndex(params.GetParamByIndex(i)) == -1)
                paramList.floatParams.Append(params.GetParamByIndex(i));
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Sets the "animation channel" which drives the sky animation.
    This could be something like "time", but the actual names are totally
    up to the application. The actual channel value will be pulled from
    the render context.
*/
void
nSkyNode::SetChannel(const char* name)
{
    n_assert(name);
    this->HChannel = nVariableServer::Instance()->GetVariableHandleByName(name);
}

//------------------------------------------------------------------------------
/**
    Return the animation channel which drives the sky animation.
*/
const char*
nSkyNode::GetChannel()
{
    if (nVariable::InvalidHandle != this->HChannel)
    {
        return nVariableServer::Instance()->GetVariableName(this->HChannel);
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    Return actual Camera Position
*/
vector3
nSkyNode::GetCameraPos()
{
    matrix44 viewMatrix = nGfxServer2::Instance()->GetTransform(nGfxServer2::View);
    matrix44 _viewMatrix = viewMatrix;
    _viewMatrix.pos_component() = vector3::Zero;
    _viewMatrix.invert();
    vector3 cameraPos;
    _viewMatrix.mult(viewMatrix.pos_component(), cameraPos);
    return cameraPos;
}
