#include "UserProfile.h"

#include <StdDEM.h>

namespace Profiles
{
ImplementRTTI(Profiles::UserProfile, Core::CRefCounted);
ImplementFactory(Profiles::UserProfile);

UserProfile::UserProfile(): _IsOpen(false)
{
}
//---------------------------------------------------------------------

UserProfile::~UserProfile()
{
	if (IsOpen()) Close();
}
//---------------------------------------------------------------------

// This static method returns the path to the profiles root directory for this application.
nString UserProfile::GetProfileRootDirectory()
{
	static const nString Root("appdata:");
	return Root;
}
//---------------------------------------------------------------------

// Returns the path to the user's profile directory using the Nebula2 filesystem path conventions.
nString UserProfile::GetProfileDirectory() const
{
	nString Path;
	Path.Format("%sprofiles/%s", GetProfileRootDirectory().Get(), GetName().Get());
	return Path;
}
//---------------------------------------------------------------------

void UserProfile::SetToDefaults()
{
}
//---------------------------------------------------------------------

// This is a static method which returns the names of all user profiles which currently exist on disk.
nArray<nString> UserProfile::EnumProfiles()
{
	n_error("!!!IMPLEMENT ME!!!");
	return nArray<nString>();
	//return DataSrv->ListDirectories(GetProfileRootDirectory());
}
//---------------------------------------------------------------------

// This static method deletes an existing user profile on disk.
void UserProfile::DeleteProfile(const nString& PrfName)
{
	//!!!TODO!
	// FIXME
}
//---------------------------------------------------------------------

bool UserProfile::Open()
{
	//nString FileName;
	//FileName.Format("%s/profile.xml", this->GetProfileDirectory().Get());

	//Stream.SetFilename(FileName);
	//if (Stream.Open(nStream::ReadWrite))
	//{
	//	if (Stream.FileCreated())
	//	{
	//		Stream.BeginNode("Profile");
	//		Stream.EndNode();
	//	}
	//	Stream.SetToNode("/Profile");
	//	OK;
	//}
	//FAIL;
	OK;
}
//---------------------------------------------------------------------

// Close the profile. This will save the profile back to disc.
void UserProfile::Close()
{
	//Stream.Close();
}
//---------------------------------------------------------------------

} // namespace Loading



