#include <Data/FS/NpkTOC.h>
#include <Data/Streams/FileStream.h>
#include <Data/DataArray.h>
#include <Data/BinaryWriter.h>

void FilterByFolder(const nString& Folder, nArray<nString>& In, nArray<nString>& Out);
bool AddFilesToTOC(nArray<nString>& Files, CNpkTOC& TOCObj, int& Offset);
bool AddDirectoryToTOC(const nString& DirName, CNpkTOC& TOCObj, int& Offset);
void WriteTOCEntry(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry);
bool WriteEntryData(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry, int dataBlockOffset, int& dataLen);
bool WriteNPK(const nString& NpkName, CNpkTOC& TOCObj);

bool CopyDirectoryToBuild(LPCSTR From, LPCSTR To)
{
	Data::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(From))
	{
		n_printf("Could not open directory '%s' for reading!\n", From);
		FAIL;
	}

	DataSrv->CreateDirectory(To);

	if (!Browser.IsCurrDirEmpty()) do
	{
		Data::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

		if (CurrEntryType == Data::FSE_FILE)
		{
			const nString& FileName = Browser.GetCurrEntryName();
			n_assert(DataSrv->CopyFile(From + FileName, To + FileName));
			n_printf("%s: %s -> %s\n", FileName.Get(), From, To);
		}

		// Ignore subdirectories for now
		//else if (CurrEntryType == Data::FSE_DIR)
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	if (BuildDir.IsEmpty()) BuildDir = "home:Build";
	
	DataSrv->SetAssign("proj", ProjDir);
	DataSrv->SetAssign("build", BuildDir);

	//!!!to script config!
	nString Proj = DataSrv->ManglePath("proj:");
	nString Export = Proj + "/export";
	DataSrv->SetAssign("shaders", "home:shaders");
	DataSrv->SetAssign("renderpath", "home:shaders");
	DataSrv->SetAssign("export", Export);
	DataSrv->SetAssign("src", Proj + "/src");
	DataSrv->SetAssign("scene", Export + "/Scene");
	DataSrv->SetAssign("scenesrc", "src:Scene");
	DataSrv->SetAssign("dlg", Export + "/game/dlg");
	DataSrv->SetAssign("dlgsrc", "src:game/dlg");
	DataSrv->SetAssign("physics", Export + "/physics");
	DataSrv->SetAssign("meshes", Export + "/meshes");
	DataSrv->SetAssign("materials", Export + "/materials");
	DataSrv->SetAssign("mtlsrc", Export + "/materials");
	DataSrv->SetAssign("textures", Export + "/textures");
	DataSrv->SetAssign("anims", Export + "/anims");

	//!!!DBG TMP!
	/*{
		Data::CFileStream File;
		if (File.Open("anims:examples/Door.kfa", Data::SAM_WRITE))
		{
			Data::CBinaryWriter Wr(File);
			Wr.Write('KFAN');
			Wr.Write(0.5f);
			Wr.Write<DWORD>(1);
			Wr.Write<LPCSTR>("DoorBody");
			Wr.Write((int)Anim::Chnl_Rotation);
			Wr.Write<DWORD>(2);
			quaternion q;
			Wr.Write(q);
			Wr.Write(0.f);
			q.set_rotate_y(PI / 2.f);
			Wr.Write(q);
			Wr.Write(0.5f);
			File.Close();
		}
	}*/

	n_printf("\n-----------------------------------------------------\n");

	n_printf("\nCompiling HRD & Lua sources:\n");
	CompileAllHRD("materials", "hrd", "prm");
	CompileAllLua("game/quests", "lua", "lua", NULL);

	n_printf("\n-----------------------------------------------------\n");

	// Analyze materials for textures

	for (int i = MaterialFiles.Size() - 1; i >= 0; i--)
	{
		nString& FileName = MaterialFiles[i];
		n_printf("\nParsing material '%s'...\n", FileName.Get());

		PParams Mtl = DataSrv->LoadPRM(FileName, false);

		if (!Mtl.isvalid())
		{
			n_printf("WARNING: material not found, builder deleted it from the list.\nBuild may be invalid!\n");
			MaterialFiles.Erase(i);
			continue;
		}

		CData* pValue;
		if (Mtl->Get(pValue, CStrID("Textures")))
		{
			if (pValue->IsA<PParams>())
			{
				const CParams& Textures = *pValue->GetValue<PParams>();
				for (int i = 0; i < Textures.GetCount(); ++i)
					AddRsrcIfUnique(Textures.Get(i).GetValue<nString>(), ResourceFiles, "Texture");
			}
		}
	}

	ResourceFiles.AppendArray(MaterialFiles);
	MaterialFiles.Clear();

	n_printf("\n-----------------------------------------------------\n");

	n_printf("\nAdding system resources:\n");
	AddRsrcIfUnique("textures:system/noise.dds", ResourceFiles, "Texture");

	n_printf("\n-----------------------------------------------------\n\nPacking...\n\n");
	
	for (int i = 0; i < ResourceFiles.Size(); i++)
		ResourceFiles[i] = DataSrv->ManglePath(ResourceFiles[i]);

	nArray<nString> RsrcFromExport;
	FilterByFolder(DataSrv->ManglePath("export:"), ResourceFiles, RsrcFromExport);
	n_assert(ResourceFiles.Size() == 0);

	n_printf("Building table of contents...\n");

	nString DestFile("build:export.npk");
	CNpkTOC TOC;
	TOC.SetRootPath(ProjDir.Get());

	int Offset = 0;

	TOC.BeginDirEntry("export");

	if (!AddDirectoryToTOC("cegui", TOC, Offset)) goto error;

	if (!AddDirectoryToTOC("db", TOC, Offset)) goto error;

	// FilterByFolder _appends_ recs within a folder from In arrray to Out array

	TOC.BeginDirEntry("scene");
	FilterByFolder("scene", RsrcFromExport, SceneFiles2);
	AddFilesToTOC(SceneFiles2, TOC, Offset);
	TOC.EndDirEntry();

	TOC.BeginDirEntry("physics");
	FilterByFolder("physics", RsrcFromExport, PhysicsFiles);
	AddFilesToTOC(PhysicsFiles, TOC, Offset);
	TOC.EndDirEntry();

	TOC.BeginDirEntry("nav");
	FilterByFolder("nav", RsrcFromExport, NavMeshFiles);
	AddFilesToTOC(NavMeshFiles, TOC, Offset);
	TOC.EndDirEntry();

	TOC.BeginDirEntry("game");
	{
		nArray<nString> GameFiles;
		FilterByFolder("game", RsrcFromExport, GameFiles);

		TOC.BeginDirEntry("anim");
		FilterByFolder("anim", GameFiles, AnimDescFiles);
		AddFilesToTOC(AnimDescFiles, TOC, Offset);
		TOC.EndDirEntry();

		TOC.BeginDirEntry("iao");
		FilterByFolder("iao", GameFiles, IAODescFiles);
		AddFilesToTOC(IAODescFiles, TOC, Offset);
		TOC.EndDirEntry();

		TOC.BeginDirEntry("dlg");
		FilterByFolder("dlg", GameFiles, DlgFiles);
		AddFilesToTOC(DlgFiles, TOC, Offset);
		TOC.EndDirEntry();

		TOC.BeginDirEntry("ai");
		{
			nArray<nString> AIFiles;
			FilterByFolder("ai", GameFiles, AIFiles);

			TOC.BeginDirEntry("actors");
			FilterByFolder("actors", AIFiles, ActorDescFiles);
			AddFilesToTOC(ActorDescFiles, TOC, Offset);
			TOC.EndDirEntry();

			TOC.BeginDirEntry("hints");
			FilterByFolder("hints", AIFiles, AIHintsDescFiles);
			AddFilesToTOC(AIHintsDescFiles, TOC, Offset);
			TOC.EndDirEntry();

			TOC.BeginDirEntry("smarts");
			FilterByFolder("smarts", AIFiles, SmartObjDescFiles);
			AddFilesToTOC(SmartObjDescFiles, TOC, Offset);
			TOC.EndDirEntry();

			//if (!AddDirectoryToTOC("bhv", TOC, Offset)) goto error;
			
			AddFilesToTOC(AIFiles, TOC, Offset);
		}
		TOC.EndDirEntry();

		CompileAllHRD("game/items", "hrd", "prm");
		if (!AddDirectoryToTOC("items", TOC, Offset)) goto error;
	
		CompileAllHRD("game/quests", "hrd", "prm");
		if (!AddDirectoryToTOC("quests", TOC, Offset)) goto error;

		if (!AddDirectoryToTOC("scripts", TOC, Offset)) goto error;
		
		AddFilesToTOC(GameFiles, TOC, Offset);
	}
	TOC.EndDirEntry();

	AddFilesToTOC(RsrcFromExport, TOC, Offset);

	TOC.EndDirEntry();

	n_printf("Done\n");

	if (!WriteNPK(DestFile, TOC))
	{
		n_printf("ERROR IN FILE GENERATION, DELETING NPK FILE\n");
		DataSrv->DeleteFile(DestFile);
		goto error;
	}
	else
	{
		n_printf("\n\nNPK file:      %s\nNPK file size: %.3f MB\n",
			DataSrv->ManglePath(DestFile).Get(),
			DataSrv->GetFileSize(DestFile) / (1024.f * 1024.f));
	}

	//!!!!!!!!!!!!
	//!!!!!!!!Copy all shaders, project tables, init scripts
	//(or just project tables as shaders & scripts are changed manually)?
	//!!!!!!!!!!!!

	n_printf("\n-----------------------------------------------------\n\nCopying system data files...\n\n");

	if (!CopyDirectoryToBuild("proj:Project/tables/", "build:data/tables/")) goto error;
	if (!CopyDirectoryToBuild("proj:Project/Input/", "build:data/Input/")) goto error;

	n_printf("\n-----------------------------------------------------\n");
	
	n_printf("Building done.\n");
	if (WaitKey)
	{
		n_printf("Press any key to exit...\n");
		getch();
	}

	Release();
	return 0;

error:

	return FailApp(WaitKey);
}
//---------------------------------------------------------------------
