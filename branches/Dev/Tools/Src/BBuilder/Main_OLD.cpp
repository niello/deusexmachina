#include <Core/CoreServer.h>
#include <Data/FS/NpkTOC.h>
#include <Data/Streams/FileStream.h>
#include <DB/DBServer.h>
#include <DB/Database.h>
#include <DB/Dataset.h>
#include <Data/DataArray.h>
#include <Data/BinaryWriter.h>

Ptr<DB::CDBServer>		DBServer;

void FilterByFolder(const nString& Folder, nArray<nString>& In, nArray<nString>& Out);
bool AddFilesToTOC(nArray<nString>& Files, CNpkTOC& TOCObj, int& Offset);
bool AddDirectoryToTOC(const nString& DirName, CNpkTOC& TOCObj, int& Offset);
void WriteTOCEntry(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry);
bool WriteEntryData(Data::CFileStream* pFile, CNpkTOCEntry* tocEntry, int dataBlockOffset, int& dataLen);
bool WriteNPK(const nString& NpkName, CNpkTOC& TOCObj);

bool LuaCompile(char* pData, uint Size, LPCSTR Name, LPCSTR pFileOut);
bool LuaCompileClass(Data::CParams& LoadedHRD, LPCSTR Name, LPCSTR pFileOut);
void LuaRelease();

bool Init()
{
	n_new(Core::CCoreServer());
	CoreSrv->Open();
	//!!!some unused servers are created inside CoreServer->Open()! refactor!

	OK;
}
//---------------------------------------------------------------------

bool AddRsrcIfUnique(const nString& Rsrc, nArray<nString>& Array, const char* Category)
{
	if (Rsrc.IsValid() && Array.BinarySearchIndex(Rsrc) == INVALID_INDEX)
	{
		Array.InsertSorted(Rsrc);
		n_printf("  + %s: %s\n", Category, Rsrc.Get());
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool AddRsrcIfUnique(const DB::PDataset& DS, DB::CAttrID AttrID, nArray<nString>& Array, const nString& Ext = NULL)
{
	if (DS->GetValueTable()->HasColumn(AttrID))
	{
		const nString& AttrValue = DS->Get<nString>(AttrID);
		if (AttrValue.IsValid())
			return AddRsrcIfUnique(AttrValue + Ext, Array, AttrID->GetName().CStr());
	}
	FAIL;
}
//---------------------------------------------------------------------

bool ParseSceneNode(const CParams& NodeDesc, nArray<nString>& ResourceFiles)
{
	CData* pValue;
	if (NodeDesc.Get(pValue, CStrID("Textures")))
	{
		if (pValue->IsA<PParams>())
		{
			const CParams& Textures = *pValue->GetValue<PParams>();
			for (int i = 0; i < Textures.GetCount(); ++i)
				AddRsrcIfUnique(Textures.Get(i).GetValue<nString>(), ResourceFiles, "Texture");
		}
	}

	if (NodeDesc.Get(pValue, CStrID("Mesh")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "Mesh");

	if (NodeDesc.Get(pValue, CStrID("Anim")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "Animation");

	if (NodeDesc.Get(pValue, CStrID("ChunkFile")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "ChunkLODMesh");

	if (NodeDesc.Get(pValue, CStrID("TQTFile")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "Tqt2Texture");

	if (NodeDesc.Get(pValue, CStrID("Children")))
	{
		const CParams& Children = *pValue->GetValue<PParams>();
		for (int i = 0; i < Children.GetCount(); ++i)
			if (!ParseSceneNode(*Children[i].GetValue<PParams>(), ResourceFiles)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool ParseDEMSceneNode(const CParams& NodeDesc, nArray<nString>& ResourceFiles, nArray<nString>& MaterialFiles)
{
	CData* pValue;
	if (NodeDesc.Get(pValue, CStrID("Textures")))
	{
		if (pValue->IsA<PParams>())
		{
			const CParams& Textures = *pValue->GetValue<PParams>();
			for (int i = 0; i < Textures.GetCount(); ++i)
				AddRsrcIfUnique(Textures.Get(i).GetValue<nString>(), ResourceFiles, "Texture");
		}
	}

	if (NodeDesc.Get(pValue, CStrID("Material")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), MaterialFiles, "Material");

	if (NodeDesc.Get(pValue, CStrID("Mesh")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "Mesh");

	if (NodeDesc.Get(pValue, CStrID("Anim")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "Animation");

	if (NodeDesc.Get(pValue, CStrID("CDLODFile")))
		AddRsrcIfUnique(pValue->GetValue<nString>(), ResourceFiles, "CDLOD");

	if (NodeDesc.Get(pValue, CStrID("Attrs")))
	{
		const CDataArray& Attrs = *pValue->GetValue<PDataArray>();
		for (int i = 0; i < Attrs.Size(); ++i)
			if (!ParseDEMSceneNode(*Attrs[i].GetValue<PParams>(), ResourceFiles, MaterialFiles)) FAIL;
	}

	if (NodeDesc.Get(pValue, CStrID("Children")))
	{
		const CParams& Children = *pValue->GetValue<PParams>();
		for (int i = 0; i < Children.GetCount(); ++i)
			if (!ParseDEMSceneNode(*Children[i].GetValue<PParams>(), ResourceFiles, MaterialFiles)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

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

void BatchCompileHRD(nArray<nString>& Files, LPCSTR Dir, LPCSTR ExtRaw, LPCSTR ExtOut)
{
	n_printf("Batch-compiling HRD in %s: %s -> %s\n", Dir, ExtRaw, ExtOut);
	static const nString Src = DataSrv->ManglePath("src:");
	static const nString Export = DataSrv->ManglePath("export:");
	nString PathSrc = Src + "/" + Dir + "/";
	nString PathExport = Export + "/" + Dir + "/";
	for (int i = Files.Size() - 1; i >= 0; i--)
	{
		nString& FileName = Files[i];
		n_printf("\nParsing HRD '%s'...\n", FileName.Get());

		nString FullName = PathSrc + FileName + ExtRaw;
		PParams Params = DataSrv->LoadHRD(FullName, false);

		if (!Params.isvalid())
		{
			n_printf("WARNING: file not found, builder deleted it from the list. Build may be invalid!\n");
			Files.Erase(i);
			continue;
		}

		FileName += ExtOut;

		FullName = PathExport + FileName;
		nString FullPath = FullName.ExtractDirName();
		if (!DataSrv->DirectoryExists(FullPath)) DataSrv->CreateDirectory(FullPath);

		//???!!!use scheme?!
		DataSrv->SavePRM(FullName, Params);
	}
}
//---------------------------------------------------------------------

void CompileAllHRD(LPCSTR Dir, LPCSTR ExtRaw, LPCSTR ExtOut)
{
	n_printf("Compiling all HRD in %s: %s -> %s\n", Dir, ExtRaw, ExtOut);

	static const nString Src = DataSrv->ManglePath("src:");
	static const nString Export = DataSrv->ManglePath("export:");
	nString PathSrc = Src + "/" + Dir + "/";
	nString PathExport = Export + "/" + Dir + "/";
	nString DotExtOut = nString(".") + ExtOut;

	Data::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(PathSrc))
	{
		n_printf("Could not open directory '%s' for reading!\n", PathSrc.Get());
		return;
	}

	DataSrv->CreateDirectory(PathExport);

	if (!Browser.IsCurrDirEmpty()) do
	{
		Data::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

		const nString& FileName = Browser.GetCurrEntryName();
		if (CurrEntryType == Data::FSE_FILE)
		{
			if (FileName.CheckExtension(ExtRaw))
			{
				n_printf("Parsing HRD '%s'...\n", FileName.Get());

				nString FullName = PathSrc + FileName;
				PParams Params = DataSrv->LoadHRD(FullName, false);

				if (!Params.isvalid())
				{
					n_printf("WARNING: Invalid HRD!\n");
					continue;
				}

				nString FileNoExt = FileName;
				FileNoExt.StripExtension();

				//???!!!use scheme?!
				DataSrv->SavePRM(PathExport + FileNoExt + DotExtOut, Params);
			}
		}
		else if (CurrEntryType == Data::FSE_DIR)
		{
			nString SubDir(Dir);
			SubDir += "/";
			SubDir += FileName;
			CompileAllHRD(SubDir.Get(), ExtRaw, ExtOut);
		}
	}
	while (Browser.NextCurrDirEntry());
}
//---------------------------------------------------------------------

void CompileAllLuaClasses(LPCSTR Dir, LPCSTR ExtRaw, LPCSTR ExtOut)
{
	n_printf("Compiling all Lua in %s: %s -> %s\n", Dir, ExtRaw, ExtOut);

	static const nString Src = DataSrv->ManglePath("src:");
	static const nString Export = DataSrv->ManglePath("export:");
	nString PathSrc = Src + "/" + Dir + "/";
	nString PathExport = Export + "/" + Dir + "/";
	nString DotExtOut = nString(".") + ExtOut;

	Data::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(PathSrc))
	{
		n_printf("Could not open directory '%s' for reading!\n", PathSrc.Get());
		return;
	}

	DataSrv->CreateDirectory(PathExport);

	if (!Browser.IsCurrDirEmpty()) do
	{
		Data::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

		const nString& FileName = Browser.GetCurrEntryName();
		if (CurrEntryType == Data::FSE_FILE)
		{
			if (FileName.CheckExtension(ExtRaw))
			{
				n_printf("Compiling Lua class '%s'...\n", FileName.Get());
				nString FileNoExt = FileName;
				FileNoExt.StripExtension();

				PParams HRD = DataSrv->LoadHRD(PathSrc + FileName, false);
				if (!HRD.isvalid()) n_printf("Invalid class file\n");

				if (!LuaCompileClass(*HRD, FileNoExt.Get(), (PathExport + FileNoExt + DotExtOut).Get()))
					n_printf("Error, not compiled\n");
			}
		}
		else if (CurrEntryType == Data::FSE_DIR)
		{
			nString SubDir(Dir);
			SubDir += "/";
			SubDir += FileName;
			CompileAllLuaClasses(SubDir.Get(), ExtRaw, ExtOut);
		}
	}
	while (Browser.NextCurrDirEntry());
}
//---------------------------------------------------------------------

void CompileAllLua(LPCSTR Dir, LPCSTR ExtRaw, LPCSTR ExtOut, LPCSTR pClassesFolder)
{
	n_printf("Compiling all Lua in %s: %s -> %s\n", Dir, ExtRaw, ExtOut);

	static const nString Src = DataSrv->ManglePath("src:");
	static const nString Export = DataSrv->ManglePath("export:");
	nString PathSrc = Src + "/" + Dir + "/";
	nString PathExport = Export + "/" + Dir + "/";
	nString DotExtOut = nString(".") + ExtOut;

	Data::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(PathSrc))
	{
		n_printf("Could not open directory '%s' for reading!\n", PathSrc.Get());
		return;
	}

	DataSrv->CreateDirectory(PathExport);

	if (!Browser.IsCurrDirEmpty()) do
	{
		Data::EFSEntryType CurrEntryType = Browser.GetCurrEntryType();

		const nString& FileName = Browser.GetCurrEntryName();
		if (CurrEntryType == Data::FSE_FILE)
		{
			if (FileName.CheckExtension(ExtRaw))
			{
				n_printf("Compiling Lua '%s'...\n", FileName.Get());
				nString FileNoExt = FileName;
				FileNoExt.StripExtension();

				Data::CBuffer Buffer;
				if (DataSrv->LoadFileToBuffer(PathSrc + FileName, Buffer))
				{
					if (!LuaCompile((char*)Buffer.GetPtr(), Buffer.GetSize(), FileNoExt.Get(), (PathExport + FileNoExt + DotExtOut).Get()))
						n_printf("Error, not compiled\n");
				}
				else n_printf("Can't open file\n");
			}
		}
		else if (CurrEntryType == Data::FSE_DIR && (!pClassesFolder || FileName != pClassesFolder))
		{
			nString SubDir(Dir);
			SubDir += "/";
			SubDir += FileName;
			CompileAllLua(SubDir.Get(), ExtRaw, ExtOut, NULL);
		}
	}
	while (Browser.NextCurrDirEntry());
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	if (StaticDBFile.IsEmpty() && GameDBFile.IsEmpty()) return 1;

	if (BuildDir.IsEmpty()) BuildDir = "home:Build";

	if (GameDBFile == StaticDBFile) StaticDBFile = NULL;

	if (!Init())
	{
		n_error("BBuilder: Failed to initialize runtime");
		Release();
		return 1;
	}
	
	n_printf("BBuilder v"VERSION" for DeusExMachina engine, (c) Vladimir \"Niello\" Orlov 2011\n");
	
	ProjDir.ConvertBackslashes();
	ProjDir.StripTrailingSlash();
	
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

	PParams Cats = DataSrv->LoadHRD("proj:Project/tables/EntityCats.hrd", false);

	if (!Cats.isvalid())
	{
		n_error("BBuilder: Failed to read 'proj:Project/tables/EntityCats.hrd'");
		return FailApp(WaitKey);
	}

	// Analyze DB(s) and get names of used resources

	nArray<nString> SceneFiles2;
	nArray<nString> AnimDescFiles;
	nArray<nString> PhysicsFiles;
	nArray<nString> IAODescFiles;
	nArray<nString> ActorDescFiles;
	nArray<nString> AIHintsDescFiles;
	nArray<nString> SmartObjDescFiles;
	nArray<nString> DlgFiles;
	nArray<nString> NavMeshFiles;
	nArray<nString> MaterialFiles;

	RegisterN2SQLiteVFS();

	if (StaticDBFile.IsValid())
	{
		n_printf("\nEstablishing DB connection to '%s'...\n", StaticDBFile.Get());
		n_printf("-----------------------------------------------------\n");
		
		DB::PDatabase DB = OpenDB("export:db/" + StaticDBFile);
		
		const CStrID sTplTable("TplTableName");
		for (int i = 0; i < Cats->GetCount(); i++)
		{
			PParams Cat = Cats->Get<PParams>(i);
			nString DefaultTbl("Tpl");
			DefaultTbl += Cats->Get(i).GetName().CStr();
			nString TblName = Cat->Get<nString>(sTplTable, DefaultTbl);
			if (TblName.IsEmpty()) continue;

			int Idx = DB->FindTableIndex(TblName);
			if (Idx != INVALID_INDEX)
			{
				n_printf("\nParsing table '%s'...\n", TblName.Get());
				
				DB::PDataset DS = DB->GetTable(Idx)->CreateDataset();
				DS->AddColumnsFromTable();
				DS->PerformQuery();

				for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
				{
					DS->SetRowIndex(i);
					
					AddRsrcIfUnique(DS, Attr::SceneFile, SceneFiles2);
					AddRsrcIfUnique(DS, Attr::AnimDesc, AnimDescFiles);
					AddRsrcIfUnique(DS, Attr::Physics, PhysicsFiles);
					AddRsrcIfUnique(DS, Attr::IAODesc, IAODescFiles);
					AddRsrcIfUnique(DS, Attr::ActorDesc, ActorDescFiles);
					AddRsrcIfUnique(DS, Attr::AIHintsDesc, AIHintsDescFiles);
					AddRsrcIfUnique(DS, Attr::SmartObjDesc, SmartObjDescFiles);
					AddRsrcIfUnique(DS, Attr::Dialogue, DlgFiles);
				}
			}
		}
		
		DB->Close();
		n_assert(DB->GetRefCount() == 1);
		n_printf("\nDB connection to '%s' closed\n", StaticDBFile.Get());
		n_printf("-----------------------------------------------------\n\n");
	}

	if (GameDBFile.IsValid())
	{
		n_printf("Establishing DB connection to '%s'...\n", GameDBFile.Get());
		n_printf("-----------------------------------------------------\n");

		DB::PDatabase DB = OpenDB("export:db/" + GameDBFile);

		int Idx = DB->FindTableIndex("Levels");
		if (Idx != INVALID_INDEX)
		{
			n_printf("\nParsing table 'Levels'...\n");
			
			DB::PDataset DS = DB->GetTable(Idx)->CreateDataset();
			DS->AddColumnsFromTable();
			DS->PerformQuery();

			//n_assert(DS->GetValueTable()->HasColumn(Attr::NavMesh));

			for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
			{
				DS->SetRowIndex(i);
				AddRsrcIfUnique(DS, Attr::NavMesh, NavMeshFiles, ".nm");
			}
		}

		const CStrID sInstTable("InstTableName");
		for (int i = 0; i < Cats->GetCount(); i++)
		{
			PParams Cat = Cats->Get<PParams>(i);
			nString DefaultTbl("Inst");
			DefaultTbl += Cats->Get(i).GetName().CStr();
			nString TblName = Cat->Get<nString>(sInstTable, DefaultTbl);
			if (TblName.IsEmpty()) continue;

			Idx = DB->FindTableIndex(TblName);
			if (Idx != INVALID_INDEX)
			{
				n_printf("\nParsing table '%s'...\n", TblName.Get());
				
				DB::PDataset DS = DB->GetTable(Idx)->CreateDataset();
				DS->AddColumnsFromTable();
				DS->PerformQuery();

				for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
				{
					DS->SetRowIndex(i);
					
					AddRsrcIfUnique(DS, Attr::SceneFile, SceneFiles2);
					AddRsrcIfUnique(DS, Attr::AnimDesc, AnimDescFiles);
					AddRsrcIfUnique(DS, Attr::Physics, PhysicsFiles);
					AddRsrcIfUnique(DS, Attr::IAODesc, IAODescFiles);
					AddRsrcIfUnique(DS, Attr::ActorDesc, ActorDescFiles);
					AddRsrcIfUnique(DS, Attr::AIHintsDesc, AIHintsDescFiles);
					AddRsrcIfUnique(DS, Attr::SmartObjDesc, SmartObjDescFiles);
					AddRsrcIfUnique(DS, Attr::Dialogue, DlgFiles);
				}
			}
		}
		
		DB->Close();
		n_assert(DB->GetRefCount() == 1);
		n_printf("\nDB connection to '%s' closed\n", GameDBFile.Get());
		n_printf("-----------------------------------------------------\n\n");
	}

	UnregisterN2SQLiteVFS();

	Cats = NULL;

	// Analyze used N2 files to get textures, meshes etc

	n_printf("\n-----------------------------------------------------\n");

	nArray<nString> ResourceFiles;

	if (!DataSrv->LoadDataSchemes("home:DataSchemes/SceneNodes.dss"))
	{
		n_error("BBuilder: Failed to read 'home:DataSchemes/SceneNodes.dss'");
		return FailApp(WaitKey);
	}

	Data::PDataScheme SceneRsrcScheme = DataSrv->GetDataScheme(CStrID("SceneNode"));

	for (int i = 0; i < SceneFiles2.Size(); i++)
	{
		nString& SceneFile = SceneFiles2[i];

		n_printf("\nParsing scene resource '%s'...\n", SceneFile.Get());

		nString SceneRsrcName = "scenesrc:" + SceneFile + ".hrd";

		PParams SceneRsrc = DataSrv->LoadHRD(SceneRsrcName, false);
		if (SceneRsrc.isvalid())
		{
			if (!ParseDEMSceneNode(*SceneRsrc, ResourceFiles, MaterialFiles))
			{
				n_printf("BBuilder: Failed to parse scene resource '%s'\n", SceneRsrcName.Get());
				return FailApp(WaitKey);
			}

			SceneFile = SceneFile + ".scn";

			nString FullPath = ("scene:" + SceneFile).ExtractDirName();
			if (!DataSrv->DirectoryExists(FullPath)) DataSrv->CreateDirectory(FullPath);

			CFileStream File;
			if (File.Open("scene:" + SceneFile, SAM_WRITE))
			{
				CBinaryWriter Writer(File);
				Writer.WriteParams(*SceneRsrc, *SceneRsrcScheme);
				File.Close();
			}
		}
	}

	SceneRsrcScheme = NULL;

	n_printf("\n-----------------------------------------------------\n");

	// Analyze dialogue files for corresponding scripts
	
	for (int i = DlgFiles.Size() - 1; i >= 0; i--)
	{
		nString& FileName = DlgFiles[i];
		n_printf("\nParsing dialogue '%s'...\n", FileName.Get());

		nString FullName = "dlgsrc:" + FileName + ".dlg";
		PParams Params = DataSrv->LoadHRD(FullName, false);

		if (!Params.isvalid())
		{
			n_printf("WARNING: dialogue not found, builder deleted it from the list.\nBuild may be invalid!\n");
			DlgFiles.Erase(i);
			continue;
		}

		nString FullPath = ("dlg:" + FileName).ExtractDirName();
		if (!DataSrv->DirectoryExists(FullPath)) DataSrv->CreateDirectory(FullPath);

		//???!!!use scheme?!
		DataSrv->SavePRM("dlg:" + FileName + ".prm", Params);
		
		bool UsesScript = false;

		const CDataArray& Links = *(Params->Get<PDataArray>(CStrID("Links")));
		for (int j = 0; j < Links.Size(); j++)
		{
			//const CDataArray& Link = *(Links[j]); // Crashes vs2008 :)
			const CDataArray& Link = *(Links.Get(j).GetValue<PDataArray>());

			if (Link.Size() > 2)
			{
				if (Link.Get(2).GetValue<nString>().IsValid()) UsesScript = true;
				else if (Link.Size() > 3 && Link.Get(3).GetValue<nString>().IsValid()) UsesScript = true;
				if (UsesScript)
				{
					int Idx = Params->IndexOf(CStrID("ScriptFile"));
					nString ScriptFile = (Idx == INVALID_INDEX) ? ("dlg:" + FileName + ".lua") : Params->Get(Idx).GetValue<nString>();
					AddRsrcIfUnique(ScriptFile, ResourceFiles, "Dialogue Script");
					break;
				}
			}
		}

		FileName += ".prm";
	}

	n_printf("\n-----------------------------------------------------\n");
	
	for (int i = PhysicsFiles.Size() - 1; i >= 0; i--)
	{
		nString& FileName = PhysicsFiles[i];
		n_printf("\nParsing physics descriptor '%s'...\n", FileName.Get());

		nString FullName = "src:physics/" + FileName + ".hrd";
		PParams Desc = DataSrv->LoadHRD(FullName, false);

		if (!Desc.isvalid())
		{
			n_printf("WARNING: physics descriptor not found, builder deleted it from the list.\nBuild may be invalid!\n");
			PhysicsFiles.Erase(i);
			continue;
		}

		FileName += ".prm";

		nString FullPath = ("physics:" + FileName).ExtractDirName();
		if (!DataSrv->DirectoryExists(FullPath)) DataSrv->CreateDirectory(FullPath);

		//???!!!use scheme?!
		DataSrv->SavePRM("physics:" + FileName, Desc);

		int Idx = Desc->IndexOf(CStrID("Bodies"));
		if (Idx != INVALID_INDEX)
		{
			CDataArray& Bodies = *Desc->Get<PDataArray>(Idx);
			for (int i = 0; i < Bodies.Size(); i++)
			{
				PParams BodyDesc = Bodies[i];
				CDataArray& Shapes = *BodyDesc->Get<PDataArray>(CStrID("Shapes"));
				for (int j = 0; j < Shapes.Size(); j++)
				{
					PParams ShapeDesc = Shapes[j];
					nString Type = ShapeDesc->Get<nString>(CStrID("Type"));
					if (Type == "MeshShape" || Type == "HeightfieldShape")
						AddRsrcIfUnique(ShapeDesc->Get<nString>(CStrID("FileName")), ResourceFiles, "Collision Geom");
				}
			}
		}

		Idx = Desc->IndexOf(CStrID("Shapes"));
		if (Idx != INVALID_INDEX)
		{
			CDataArray& Shapes = *Desc->Get<PDataArray>(Idx);
			for (int i = 0; i < Shapes.Size(); i++)
			{
				PParams ShapeDesc = Shapes[i];
				nString Type = ShapeDesc->Get<nString>(CStrID("Type"));
				if (Type == "MeshShape" || Type == "HeightfieldShape")
					AddRsrcIfUnique(ShapeDesc->Get<nString>(CStrID("File"), NULL), ResourceFiles, "Collision Geom");
			}
		}
	}

	n_printf("\n-----------------------------------------------------\n");
	
	for (int i = AnimDescFiles.Size() - 1; i >= 0; i--)
	{
		nString& FileName = AnimDescFiles[i];
		n_printf("\nParsing animation descriptor '%s'...\n", FileName.Get());

		nString FullName = "src:Game/Anim/" + FileName + ".hrd";
		PParams Desc = DataSrv->LoadHRD(FullName, false);

		if (!Desc.isvalid())
		{
			n_printf("WARNING: animation descriptor not found, builder deleted it from the list.\nBuild may be invalid!\n");
			AnimDescFiles.Erase(i);
			continue;
		}

		for (int i = 0; i < Desc->GetCount(); ++i)
			AddRsrcIfUnique(Desc->Get(i).GetValue<CStrID>().CStr(), ResourceFiles, "Animation");
	}

	n_printf("\n-----------------------------------------------------\n");

	//???!!!parse descs to include parent descs? or load all /Game/ data?

	n_printf("\n-----------------------------------------------------\n");

	n_printf("\nCompiling HRD & Lua sources:\n");
	BatchCompileHRD(AnimDescFiles, "game/Anim", ".hrd", ".prm");
	BatchCompileHRD(IAODescFiles, "game/iao", ".hrd", ".prm");
	BatchCompileHRD(ActorDescFiles, "game/ai/actors", ".hrd", ".prm");
	BatchCompileHRD(AIHintsDescFiles, "game/ai/hints", ".hrd", ".prm");
	BatchCompileHRD(SmartObjDescFiles, "game/ai/smarts", ".hrd", ".prm");
	CompileAllHRD("materials", "hrd", "prm");
	CompileAllLua("game/dlg", "lua", "lua", NULL);
	CompileAllLua("game/quests", "lua", "lua", NULL);
	CompileAllLua("game/scripts", "lua", "lua", "classes");
	CompileAllLuaClasses("game/scripts/classes", "lua", "cls");

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
