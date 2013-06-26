#include <Data/FS/NpkTOC.h>

int main(int argc, const char** argv)
{
	//!!!to config!
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

	//???!!!or parse frame shader vars?!
	AddRsrcIfUnique("Export:Textures/System/Noise.dds", ResourceFiles, "Texture");
	AddRsrcIfUnique("Export:Textures/System/White.dds", ResourceFiles, "Texture");
	AddRsrcIfUnique("Export:Textures/System/NoBump.dds", ResourceFiles, "Texture");

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

	if (!CopyDirectoryToBuild("Proj:Project/Input/", "Build:Data/Input/")) goto error;
	if (!CopyDirectoryToBuild("Proj:Project/Shaders/", "Build:Data/Shaders/")) goto error;
}
//---------------------------------------------------------------------
