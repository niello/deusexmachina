#include "CharacterProfile.h"

#include <Loading/LoaderServer.h>

namespace Profiles
{
ImplementRTTI(Profiles::CharacterProfile, Profiles::UserProfile);
ImplementFactory(Profiles::CharacterProfile);

CharacterProfile::~CharacterProfile()
{
    if (IsOpen()) Close();
}
//---------------------------------------------------------------------

bool CharacterProfile::Open(const nString& PrfName)
{
	//nString FName;
	//FName.Format("%s/character/%s.xml", LoaderSrv->GetUserProfile()->GetProfileDirectory(), PrfName);

	//Stream.SetFilename(FName);
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

} // namespace Loading