#ifndef N_RESOURCE_H
#define N_RESOURCE_H
//------------------------------------------------------------------------------
/**
    @class nResource
    @ingroup Resource

    Super class for all resource classes. Resources are mostly loaded
    from a file, can be shared, and unloaded to free runtime space.
    They are managed by a central resource server.

    The nResource class provides an interface for asynchronous loading
    (loading resources in a background thread). Not all subclasses may
    support asynchronous resource loading though.

    Default behavior is synchronous loading (Load() returns when the
    resource has been loaded). Asynchronous loading is enabled by
    calling SetAsyncEnabled(true). When async loading is enabled,
    Load() returns immediately with a return value of true but IsValid()
    will return false until the resource has actually been loaded at some
    point in the future.

    To check whether a resource loading request has been issued at
    some point in the past which has not been fullfilled yet, call
    the IsPending() method. IsPending() returns true after an async
    call to Load() and before IsValid() returns true to indicate the
    the resource is available. In all other cases, IsPending() returns
    false.

    Unloading resources always happened immediately, both in sync and
    async mode.

    A resource object can be in one of the following states:

    - <b>Unloaded:</b> The resource is not currently loaded, resource
      object are in this state before Load() and after Unload()

    - <b>Valid:</b> The resource object has been loaded and is in a
      valid state to be used.

    - <b>Lost:</b> The resource has been loaded at some time in the
      past, but is currently in a Lost state, so that it can't be used.
      This can happen for D3D-controlled resources which are not placed
      in D3D's managed resource pool. Usually, applications don't need to
      be concerned about lost state, since no rendering will be performend
      while the application is in a lost state.

    - <b>Empty:</b> The resource has been initialized, but has not been filled
      with valid data. This is usually only of interest for special types of
      resources which are not loaded from disk resource files, but are filled
      by application code with data. This includes for instance procedurally
      generated meshes and textures. In general, only resources which are not
      provided with a filename can enter the Empty state. The code which uses
      the resources must check the resource for Empty state each frame before
      using the resource, and if the resource is in Empty state, initialize
      the contents of the resource, and set the resource to Valid state.

    Only resources which cannot initializes themselves from a file can go into
    Empty state, for those resources, Empty state will be entered after initializing
    the resource by calling Load(), or for resources which can go into Lost state,
    if a full-screen application loses focus (for instance through Alt-TAB).

    Resources which don't know how to load themselves may remain in Unloaded state
    after the display has been closed and opened (this happens for instance
    on Alt-RETURN).

    For the general case (meshes and textures which load themselves from files),
    all resource management happens automatically. Special cases (where resources
    are not loaded from normal resource files) should check for
    Empty or Unloaded state before using the resources, and initialize, or load
    the resources manually (don't forget to set the resource to Valid state afterwards).

    This should only be of concern for programmers which extend Nebula2 itself.
    Client applications should NEVER have to deal with any type of
    low level resource management.

    Also check out the n ResourceLoader class, this is a very clean way to handle
    custom initialization of resources without having to check manually for an invalid
    state. If a resource loader is attached to a resource object, it will be responsible
    for loading and unloading the resource. Subsystems which do their own resource management
    may derive subclasses from n ResourceLoader.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "util/nstring.h"
#include "util/ThreadVar.h"
#include "kernel/nthread.h"
#include "kernel/nref.h"

//------------------------------------------------------------------------------
class nResourceServer;
class nResource: public nRoot
{
public:
    /// general resource types
    enum Type
    {
        Mesh          = (1<<1),     // a mesh object
        Texture       = (1<<2),     // a texture object
        Shader        = (1<<3),     // a shader object
        Animation     = (1<<4),     // an animation object
        SoundResource = (1<<5),     // a shared sound resource (CSoundResource)
        Other         = (1<<9),     // something else

        AllResourceTypes = (Mesh | Texture | Shader | Animation | SoundResource | Other),
        InvalidResourceType = (1<<30),
    };

    /// resource states
    enum State
    {
        Unloaded,       // currently not loaded
        Valid,          // currently valid
        Lost,           // currently not available
        //Empty,          // available, but contains no data
    };

    /// constructor
    nResource();
    /// destructor
    virtual ~nResource();
    /// subclasses must indicate to nResource whether async mode is supported
	virtual bool CanLoadAsync() const { return false; }
    /// set resource type
	void SetType(Type t) { type = t; }
    /// get resource type
	Type GetType() const { return type; }
    /// set absolute path to resource file
	void SetFilename(const nString& path) { filename = path; } //{ pMutex->Lock(); filename = path; pMutex->Unlock(); }
    /// get absolute filename to resource file
	nString GetFilename() { return filename; } //{ pMutex->Lock(); nString str = filename; pMutex->Unlock(); return str; }
    /// enable/disable async behavior
	void SetAsyncEnabled(bool b) { asyncEnabled = b; }
    /// get async behavior
	bool GetAsyncEnabled() const { return asyncEnabled; }
    /// is a resource loading request pending?
	bool IsPending() const { return jobNode.IsLinked(); }
    /// issue a load request
    virtual bool Load();
    /// unloads the resource
    virtual void Unload();
    /// set the valid flag
    void SetState(State s) { state = s; }
    /// get current state, this can be Lost, Restored, Valid
	State GetState() const { return state.Get(); }

	bool IsValid() const { return GetState() == Valid; }
    bool IsUnloaded() const { return GetState() == Unloaded; }
    bool IsLoaded() const { return GetState() != Unloaded; }
    bool IsLost() const { return GetState() == Lost; }
    //bool IsEmpty() const { return GetState() == Empty; }

	/// get an estimated byte size of the resource data (for memory statistics)
	virtual int GetByteSize() { return 0; }
    /// get the unique id of this resource
	uint GetUniqueId() const { return uniqueId; }

protected:
    friend class nResourceServer;

	virtual bool LoadResource() { return true; }
	virtual void UnloadResource() {}
	virtual void OnLost() {}
	virtual void OnRestored() {}

private:
    static uint uniqueIdCounter;

    nString filename;
    Type type;
    bool asyncEnabled;
    nDataNode jobNode;      // for linkage into resource server's loader job list
    CThreadVar<State> state;
    uint uniqueId;
};

inline nResource::nResource():
	type(InvalidResourceType),
	asyncEnabled(false),
	state(Unloaded),
	jobNode(this)
{
    uniqueId = ++uniqueIdCounter;
}

#endif
