#pragma once
#ifndef __DEM_L2_USER_PROFILE_H__
#define __DEM_L2_USER_PROFILE_H__

#include <Core/RefCounted.h>

// An user profile represents a storage where all user specific
// data is kept across application restarts. This usually includes
// save games, options, and other per-user data. Mangalore applications should
// at least support a default profile, but everything is there to
// support more then one user profile.
// User profiles are stored in "user:[appname]/profiles/[profilename]".
// Based on mangalore UserProfile_(C) 2006 RadonLabs GmbH

namespace Profiles
{

class UserProfile: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(UserProfile);

protected:

	bool	_IsOpen;
	nString	Name;
	//nStream	Stream;

public:

	UserProfile();
	virtual ~UserProfile();

	static nArray<nString>	EnumProfiles();
	static void				DeleteProfile(const nString& PrfName);
	static nString			GetProfileRootDirectory();

	void					SetName(const nString& NewName) { Name = NewName; } //???recreate file?
	const nString&			GetName() const { return Name; }
	virtual void			SetToDefaults();
	virtual bool			Open();
	virtual void			Close();
	bool					IsOpen() const { return _IsOpen; }

	nString GetProfileDirectory() const;
	
	//bool	HasAttr(const nString& Key) const { return Stream.HasAttr(Key); }
	//void	SetString(const nString& Key, const nString& Value) { Stream.SetString(Key, Value); }
	//void	SetInt(const nString& Key, int Value) { Stream.SetInt(Key, Value); }
	//void	SetFloat(const nString& Key, float Value) { Stream.SetFloat(Key, Value); }
	//void	SetBool(const nString& Key, bool Value) { Stream.SetBool(Key, Value); }
	//void	SetVector3(const nString& Key, const vector3& Value) { Stream.SetVector3(Key, Value); }
	//void	SetVector4(const nString& Key, const vector4& Value) { Stream.SetVector4(Key, Value); }
	//nString	GetString(const nString& Key) const { return Stream.GetString(Key); }
	//int		GetInt(const nString& Key) const { return Stream.GetInt(Key); }
	//float	GetFloat(const nString& Key) const { return Stream.GetFloat(Key); }
	//bool	GetBool(const nString& Key) const { return Stream.GetBool(Key); }
	//vector3	GetVector3(const nString& Key) const { return Stream.GetVector3(Key); }
	//vector4	GetVector4(const nString& Key) const { return Stream.GetVector4(Key); }
};

RegisterFactory(UserProfile);

}

#endif
