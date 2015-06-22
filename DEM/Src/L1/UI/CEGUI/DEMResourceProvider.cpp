#include <StdCfg.h>
#include "DEMResourceProvider.h"
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>

namespace CEGUI
{

void CDEMResourceProvider::loadRawDataContainer(const String& filename, RawDataContainer& output,
												const String& resourceGroup)
{
	n_assert2(!filename.empty(), "Filename supplied for data loading must be valid");

	String FinalFilename;
	if (resourceGroup.empty()) FinalFilename = d_defaultResourceGroup + filename;
	else
	{
		int Idx = ResourceGroups.FindIndex(resourceGroup);
		if (Idx != -1) FinalFilename = ResourceGroups.ValueAt(Idx) + filename;
		else FinalFilename = filename;
	}

	IO::CFileStream File;
	if (File.Open(FinalFilename.c_str(), IO::SAM_READ))
	{
		const long Size = File.GetSize();
		unsigned char* const pBuffer = n_new_array(unsigned char, Size);
		const size_t BytesRead = File.Read(pBuffer, Size);
		if (BytesRead != Size)
		{
			n_delete_array(pBuffer);
			Sys::Error("A problem occurred while reading file: %s", FinalFilename.c_str());
		}
		File.Close();

		output.setData(pBuffer);
		output.setSize(Size);
	}
}
//---------------------------------------------------------------------

void CDEMResourceProvider::unloadRawDataContainer(RawDataContainer& data)
{
	uint8* const ptr = data.getDataPtr();
	n_delete_array(ptr);
	data.setData(0);
	data.setSize(0);
}
//---------------------------------------------------------------------

void CDEMResourceProvider::setResourceGroupDirectory(const String& resourceGroup, const String& directory)
{
	if (directory.length() == 0) return;
	const String separators("\\/");
	if (String::npos == separators.find(directory[directory.length() - 1]))
		ResourceGroups.Add(resourceGroup, directory + '/');
	else ResourceGroups.Add(resourceGroup, directory);
}
//---------------------------------------------------------------------

const String& CDEMResourceProvider::getResourceGroupDirectory(const String& resourceGroup)
{
	return ResourceGroups[resourceGroup];
}
//---------------------------------------------------------------------

void CDEMResourceProvider::clearResourceGroupDirectory(const String& resourceGroup)
{
	ResourceGroups.Clear();
}
//---------------------------------------------------------------------

size_t CDEMResourceProvider::getResourceGroupFileNames(std::vector<String>& out_vec,
														   const String& file_pattern,
														   const String& resource_group)
{
	String DirName;
	if (resource_group.empty()) DirName = d_defaultResourceGroup;
	else
	{
		int Idx = ResourceGroups.FindIndex(resource_group);
		if (Idx != -1) DirName = ResourceGroups.ValueAt(Idx);
		else DirName = "./";
	}

    size_t Entries = 0;

	CString Pattern = DirName.c_str();
	Pattern += file_pattern.c_str();

	IO::PFileSystem FS;
	CString EntryName;
	IO::EFSEntryType EntryType;
	CString RootDir(DirName.c_str());
	void* hDir = IOSrv->OpenDirectory(RootDir, NULL, FS, EntryName, EntryType);
	if (hDir)
	{
		if (EntryType != IO::FSE_NONE) do
		{
			if (EntryType == IO::FSE_FILE)
			{
				CString FullEntryName = RootDir + EntryName;
				if (FullEntryName.MatchPattern(Pattern))
				{
					out_vec.push_back(String(FullEntryName.CStr()));
					++Entries;
				}
			}
		}
		while (FS->NextDirectoryEntry(hDir, EntryName, EntryType));

		FS->CloseDirectory(hDir);
	}

	return Entries;
}
//---------------------------------------------------------------------

}