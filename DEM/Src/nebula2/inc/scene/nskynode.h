#ifndef N_SKYNODE_H
#define N_SKYNODE_H
//------------------------------------------------------------------------------
/**
    @class nSkyNode
    @ingroup Scene

    The SkyNode represents a skybox with sun, clouds and other celestial elements,
    each represented by a shapenode. The parameters can be set for different
    points of time (states), to potentiate a course of a day by interpolation.

    (C) 2005 RadonLabs GmbH
*/
#include "scene/nmaterialnode.h"
#include "gfx2/nmesh2.h"
#include "scene/nshapenode.h"
#include "scene/nlightnode.h"
#include "scene/nskystate.h"
#include "gfx2/ntexture2.h"
#include "variable/nvariable.h"

class nSkyNode : public nTransformNode
{
public:
    static const float CloudSpeedFactor;
    enum ElementType
    {
        SkyElement = 0,
        SunElement,
        LightElement,
        CloudElement,
        StarElement,
        GenericElement,

        NumElementTypes,  // keep this always at the end!
        InvalidElement
    };
    typedef struct
    {
        nRef<nSkyState> refState;
        float time;
    } StateGroup;
    typedef struct
    {
        nRef<nMaterialNode> refElement;
        nArray<StateGroup> states;
        ElementType type;
        nArray<int> linkTo;
        float refreshTime;
        float lastRefresh;
    } ElementGroup;
    typedef struct
    {
        nArray<nShaderState::Param> vectorParams;
        nArray<nShaderState::Param> floatParams;
        nArray<nShaderState::Param> intParams;
    } ParamList;

    /// constructor
    nSkyNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// Attach to sceneserver
    void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// Update
    void UpdateSky(float worldTime);
    /// Attaches a state to a sky element and defines a point of time
    int AddState(const nString& destName, const nString& stateName, float time);
    /// Detaches a state from a sky element
    void RemoveState(const nString& destName, const nString& stateName);
    /// Creates a new sky element
    void NewElement(nSkyNode::ElementType type, const nString& name);
    /// Adds an existing Object as an element
    void AddElement(nSkyNode::ElementType type, const nString& name);
    /// Deletes an element
    void DeleteElement(const nString& name);
    /// attaches one element to an other
    void LinkTo(const nString& fromName, const nString& toName);

    /// set the variable handle which drives this animator object (e.g. time)
    void SetChannel(const char* name);
    /// get the variable which drives this animator object
    const char* GetChannel();
    /// sets time multiplier
    void SetTimeFactor(float factor);
    /// gets time multiplier
    float GetTimeFactor();
    /// sets start time
    void SetStartTime(float time);
    /// gets start time
    float GetStartTime();
    /// set refresh time
    void SetRefreshTime(const nString& name, float time);
    /// get refresh time
    float GetRefreshTime(const nString& name);
    /// set time periode
    void SetTimePeriod(float periode);
    /// get time periode
    float GetTimePeriod();
    /// get actual sky time
    float GetSkyTime();

    /// Changes a statetime
    int SetStateTime(const nString& elementName, const nString& stateName, float time);
    /// Changes a statetime
    int SetStateTime(int elementNr,int stateNr, float time);
    /// Changes a statetime
    float GetStateTime(const nString& destName, const nString& stateName);
    /// Changes a statetime
    float GetStateTime(int elementNr,int stateNr);
    /// Get number of elements
    int GetNumElements();
    /// Get number of states of an element
    int GetNumStates(int elementNr);
    /// Get element (by name)
    nMaterialNode* GetElement(const nString& name);
    /// Get element (by number)
    nMaterialNode* GetElement(int elementNr);
    /// Get state (by name)
    nSkyState* GetState(const nString& elementName, const nString& stateName);
    /// Get state (by number)
    nSkyState* GetState(int elementNr, int stateNr);
    /// Get element type (by name)
    nSkyNode::ElementType GetElementType(const nString& name);
    /// Get element type (by number)
    nSkyNode::ElementType GetElementType(int elementNr);

    /// converts enum type to string
    const char* TypeToString(nSkyNode::ElementType type) const;
    /// converts a string to enum type
    nSkyNode::ElementType StringToType(const char* str) const;


protected:
    /// Adds all params from nShaderparams to param list
    void CollectParams(nShaderParams params, ParamList& paramList);
    /// Returns the index of the searched element
    int FindElement(const nString& name);
    /// Returns the index of the searched state
    int FindState(int elementNr, const nString& stateName);
    /// Positions an Element by spherical coordinates
    vector3 GetSphericalCoordinates(vector3 angles);
    /// Rotates element to face the viewer
    void SetFaceToViewer(int element);
    /// Find relevant states at a specific time for an specific element
    void FindStates(int element, float time, int& state0, int& state1);
    /// Sort out params, not to apply generic
    void SortOutParams(int element, ParamList& paramList);
    /// compute state weight
    float ComputeWeight(int element, float time, int state0, int state1);
    /// Get actual camera position
    vector3 GetCameraPos();

    nArray<ElementGroup> elements;
    float timePeriod;
    float timeFactor;
    float sunSpeedFactor;
    float startTime;
    float skyTime;
    float worldTime;
    float jumpTime;
    nVariable::Handle HChannel;
};

//------------------------------------------------------------------------------
/**
    Sets the multiplier for the time
*/
inline
void
nSkyNode::SetTimeFactor(float factor)
{
    this->timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
    Gets the multiplier for the time
*/
inline
float
nSkyNode::GetTimeFactor()
{
    return this->timeFactor;
}

//------------------------------------------------------------------------------
/**
    Sets the start point of time
*/
inline
void
nSkyNode::SetStartTime(float time)
{
    this->jumpTime = time - this->startTime;
    this->startTime = time;
}

//------------------------------------------------------------------------------
/**
    Gets the start point of time
*/
inline
float
nSkyNode::GetStartTime()
{
    return this->startTime;
}

//------------------------------------------------------------------------------
/**
    Sets the time period for a day
*/
inline
void
nSkyNode::SetTimePeriod(float period)
{
    this->timePeriod = period;
}

//------------------------------------------------------------------------------
/**
    Gets the time period for a day
*/
inline
float
nSkyNode::GetTimePeriod()
{
    return this->timePeriod;
}

//------------------------------------------------------------------------------
/**
    Gets the actual time from this skynode
*/
inline
float
nSkyNode::GetSkyTime()
{
    return this->skyTime;
}

//------------------------------------------------------------------------------
/**
    translates element types to string
*/
inline
const char*
nSkyNode::TypeToString(nSkyNode::ElementType type) const
{
    n_assert((type >= 0) && (type < nSkyNode::NumElementTypes));
    switch (type)
    {
    case nSkyNode::SkyElement:      return "sky";
    case nSkyNode::SunElement:      return "sun";
    case nSkyNode::LightElement:    return "light";
    case nSkyNode::CloudElement:    return "cloud";
    case nSkyNode::StarElement:     return "star";
    case nSkyNode::GenericElement:  return "generic";
    default:                        return "unknown";
    }
}

//------------------------------------------------------------------------------
/**
    translates string to element type
*/
inline
nSkyNode::ElementType
nSkyNode::StringToType(const char* str) const
{
    n_assert(str);
    if (strcmp(str, "sky") == 0)        return nSkyNode::SkyElement;
    if (strcmp(str, "sun") == 0)        return nSkyNode::SunElement;
    if (strcmp(str, "light") == 0)      return nSkyNode::LightElement;
    if (strcmp(str, "star") == 0)       return nSkyNode::StarElement;
    if (strcmp(str, "cloud") == 0)      return nSkyNode::CloudElement;
    if (strcmp(str, "generic") == 0)    return nSkyNode::GenericElement;
    return nSkyNode::InvalidElement;
}
//------------------------------------------------------------------------------
#endif
