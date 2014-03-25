#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryWriter.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

bool ProcessDialogue(const CString& SrcContext, const CString& ExportContext, const CString& Name)
{
	CString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".dlg", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	bool ScriptExported = false;

	const CString& ScriptFile = Desc->Get<CString>(CStrID("Script"), NULL);

	const Data::CParams& Nodes = *Desc->Get<Data::PParams>(CStrID("Nodes"));
	for (int i = 0; !ScriptExported && i < Nodes.GetCount(); ++i)
	{
		const Data::CParams& NodeDesc = *Nodes.Get<Data::PParams>(i);

		Data::PDataArray Links;
		if (!NodeDesc.Get<Data::PDataArray>(Links, CStrID("Links")) || !Links->GetCount()) continue;

		for (int j = 0; !ScriptExported && j < Links->GetCount(); ++j)
		{
			const Data::CParams& LinkDesc = *Links->Get<Data::PParams>(j);

			if (LinkDesc.Get<CString>(CStrID("Condition"), NULL).IsValid() ||
				LinkDesc.Get<CString>(CStrID("Action"), NULL).IsValid())
			{
				if (!ScriptFile.IsValid())
				{
					n_msg(VL_WARNING, "Dialogue '%s' references script functions, but hasn't associated script\n", Name.CStr());
				}
				else
				{
					ExportFilePath = CString("Scripts:") + ScriptFile + ".lua";
					if (!IsFileAdded(ExportFilePath))
					{
						if (ExportDescs) BatchToolInOut(CStrID("CFLua"), CString("SrcScripts:") + ScriptFile + ".lua", ExportFilePath);
						FilesToPack.InsertSorted(ExportFilePath);
					}
				}

				ScriptExported = true;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessCollisionShape(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	// Add terrain file for heightfield shapes (always exported) //???or allow building from L3DT src?
	CString CDLODFile = Desc->Get(CStrID("CDLODFile"), CString::Empty);
	if (CDLODFile.IsValid())
	{
		CString CDLODFilePath = "Terrain:" + CDLODFile + ".cdlod";
		if (!IsFileAdded(CDLODFilePath))
		{
			if (ExportResources &&
				!ProcessResourceDesc("SrcTerrain:" + CDLODFile + ".cfd", CDLODFilePath) &&
				!IOSrv->FileExists(CDLODFilePath))
			{
				n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", CDLODFilePath.GetExtension());
				FAIL;
			}
			FilesToPack.InsertSorted(CDLODFilePath);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessPhysicsDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	Data::PDataArray Objects;
	if (Desc->Get(Objects, CStrID("Objects")))
	{
		for (int i = 0; i < Objects->GetCount(); ++i)
		{
			Data::PParams ObjDesc = Objects->Get<Data::PParams>(i);
			CStrID PickShape = ObjDesc->Get<CStrID>(CStrID("Shape"), CStrID::Empty);
			if (PickShape.IsValid())
				if (!ProcessCollisionShape(	CString("SrcPhysics:") + PickShape.CStr() + ".hrd",
											CString("Physics:") + PickShape.CStr() + ".prm"))
				{
					n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
					FAIL;
				}
		}
	}


	OK;
}
//---------------------------------------------------------------------

bool ProcessAnimDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	for (int i = 0; i < Desc->GetCount(); ++i)
	{
		//???!!!allow compile or batch-compile?
		// can add Model resource description, associated with this anim, to CFModel list
		CString FileName = CString("Anims:") + Desc->Get(i).GetValue<CStrID>().CStr();
		if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
	}

	OK;
}
//---------------------------------------------------------------------

//???pre-unwind descs on exports?
bool ProcessDescWithParents(const CString& SrcContext, const CString& ExportContext, const CString& Name)
{
	CString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	CString BaseName = Desc->Get(CStrID("_Base_"), CString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDescWithParents(SrcContext, ExportContext, BaseName));
}
//---------------------------------------------------------------------

bool ProcessMaterialDesc(const CString& Name)
{
	CString ExportFilePath = "Materials:" + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD("SrcMaterials:" + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	Data::PParams Textures;
	if (Desc->Get(Textures, CStrID("Textures")))
	{
		//!!!when export from src, find resource desc and add source texture to CFTexture list!
		for (int i = 0; i < Textures->GetCount(); ++i)
		{
			CString FileName = CString("Textures:") + Textures->Get(i).GetValue<CStrID>().CStr();
			if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessSceneNodeRefs(const Data::CParams& NodeDesc)
{
	Data::PDataArray Attrs;
	if (NodeDesc.Get(Attrs, CStrID("Attrs")))
	{
		for (int i = 0; i < Attrs->GetCount(); ++i)
		{
			Data::PParams AttrDesc = Attrs->Get<Data::PParams>(i);

			Data::PParams Textures;
			if (AttrDesc->Get(Textures, CStrID("Textures")))
			{
				//!!!when export from src, find resource desc and add source texture to CFTexture list!
				for (int i = 0; i < Textures->GetCount(); ++i)
				{
					CString FileName = CString("Textures:") + Textures->Get(i).GetValue<CStrID>().CStr();
					if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
				}
			}

			Data::CData* pValue;

			if (AttrDesc->Get(pValue, CStrID("Material")))
				if (!ProcessMaterialDesc(pValue->GetValue<CStrID>().CStr())) FAIL;

			//!!!when export from src, find resource desc and add source Model to CFModel list!
			if (AttrDesc->Get(pValue, CStrID("Mesh")))
			{
				CString FileName = CString("Meshes:") + pValue->GetValue<CStrID>().CStr() + ".nvx2"; //!!!change extension!
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}

			//!!!when export from src, find resource desc and add source BT to CFTerrain list!
			if (AttrDesc->Get(pValue, CStrID("CDLODFile")))
			{
				CString CDLODFilePath = "Terrain:" + pValue->GetValue<CString>() + ".cdlod";
				if (!IsFileAdded(CDLODFilePath))
				{
					if (ExportResources &&
						!ProcessResourceDesc("SrcTerrain:" + pValue->GetValue<CString>() + ".cfd", CDLODFilePath) &&
						!IOSrv->FileExists(CDLODFilePath))
					{
						n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", CDLODFilePath.GetExtension());
						FAIL;
					}
					FilesToPack.InsertSorted(CDLODFilePath);
				}
			}
		}
	}

	Data::PParams Children;
	if (NodeDesc.Get(Children, CStrID("Children")))
		for (int i = 0; i < Children->GetCount(); ++i)
			if (!ProcessSceneNodeRefs(*Children->Get(i).GetValue<Data::PParams>())) FAIL;

	OK;
}
//---------------------------------------------------------------------

// Always processed from Src
bool ProcessSceneResource(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
	if (!Desc.IsValid()) FAIL;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		IO::CFileStream File;
		if (!File.Open(ExportFilePath, IO::SAM_WRITE)) FAIL;
		IO::CBinaryWriter Writer(File);
		Writer.WriteParams(*Desc, *DataSrv->GetDataScheme(CStrID("SceneNode")));
		File.Close();
	}

	FilesToPack.InsertSorted(ExportFilePath);

	return ProcessSceneNodeRefs(*Desc);
}
//---------------------------------------------------------------------

bool ProcessDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	// Some descs can be loaded twice or more from different IO path assigns, avoid it
	CString RealExportFilePath = IOSrv->ManglePath(ExportFilePath);

	if (IsFileAdded(RealExportFilePath)) OK;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(RealExportFilePath.ExtractDirName());
		if (!DataSrv->SavePRM(RealExportFilePath, DataSrv->LoadHRD(SrcFilePath, false))) FAIL;
	}
	FilesToPack.InsertSorted(RealExportFilePath);

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	CString AttrValue;

	if (Attrs->Get<CString>(AttrValue, CStrID("UIDesc")))
		if (!ProcessDesc("SrcUI:" + AttrValue + ".hrd", "UI:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing UI desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("ActorDesc")))
	{
		if (!ProcessDescWithParents("SrcActors:", "Actors:", AttrValue))
		{
			n_msg(VL_ERROR, "Error processing AI actor desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessDesc("SrcAI:AIActionTpls.hrd", "AI:AIActionTpls.prm"))
		{
			n_msg(VL_ERROR, "Error processing shared AI action templates desc\n");
			FAIL;
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("AIHintsDesc")))
		if (!ProcessDesc("SrcAIHints:" + AttrValue + ".hrd", "AIHints:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI hints desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("SODesc")))
	{
		if (!ProcessDesc("SrcSmarts:" + AttrValue + ".hrd", "Smarts:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI smart object desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessSOActionTplsDesc("SrcAI:AISOActionTpls.hrd", "AI:AISOActionTpls.prm"))
		{
			n_msg(VL_ERROR, "Error processing shared smart object action templates desc\n");
			FAIL;
		}
	}

	CStrID ItemID = Attrs->Get<CStrID>(CStrID("ItemTplID"), CStrID::Empty);
	if (ItemID.IsValid())
		if (!ProcessDesc(CString("SrcItems:") + ItemID.CStr() + ".hrd", CString("Items:") + ItemID.CStr() + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	Data::PDataArray Inventory;
	if (Attrs->Get<Data::PDataArray>(Inventory, CStrID("Inventory")))
	{
		for (int i = 0; i < Inventory->GetCount(); ++i)
		{
			ItemID = Inventory->Get<Data::PParams>(i)->Get<CStrID>(CStrID("ID"), CStrID::Empty);
			if (ItemID.IsValid() &&
				!ProcessDesc(	CString("SrcItems:") + ItemID.CStr() + ".hrd",
								CString("Items:") + ItemID.CStr() + ".prm"))
			{
				n_msg(VL_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
				FAIL;
			}
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("ScriptClass")))
	{
		CString ExportFilePath = "ScriptClasses:" + AttrValue + ".cls";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScriptClasses:" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("Script")))
	{
		CString ExportFilePath = "Scripts:" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScripts:" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	CStrID PickShape = Attrs->Get<CStrID>(CStrID("PickShape"), CStrID::Empty);
	if (PickShape.IsValid())
		if (!ProcessCollisionShape(	CString("SrcPhysics:") + PickShape.CStr() + ".hrd",
									CString("Physics:") + PickShape.CStr() + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("Physics")))
		if (!ProcessPhysicsDesc(CString("SrcPhysics:") + AttrValue + ".hrd",
								CString("Physics:") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing physics desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("AnimDesc")))
		if (!ProcessAnimDesc(	CString("SrcGameAnim:") + AttrValue + ".hrd",
								CString("GameAnim:") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing animation desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("Dialogue")))
		if (!ProcessDialogue("SrcDlg:", "Dlg:", AttrValue))
		{
			n_msg(VL_ERROR, "Error processing dialogue desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("SceneFile")))
		if (!ProcessSceneResource("SrcScene:" + AttrValue + ".hrd", "Scene:" + AttrValue + ".scn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource '%s'\n", AttrValue.CStr());
			FAIL;
		}

	OK;
}
//---------------------------------------------------------------------

bool ProcessLevel(const Data::CParams& LevelDesc, const CString& Name)
{
	// Add level script

	CString ExportFilePath = "Levels:" + Name + ".lua";
	if (ExportDescs)
	{
		CString SrcFilePath = "SrcLevels:" + Name + ".lua";
		if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
		{
			BatchToolInOut(CStrID("CFLua"), SrcFilePath, ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}
	else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Add navmesh (always exported, no src)

	ExportFilePath = "Levels:" + Name + ".nm";
	if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Export entities

	Data::PParams SubDesc;
	if (LevelDesc.Get(SubDesc, CStrID("Entities")))
	{
		for (int i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();
			n_msg(VL_INFO, " Processing entity '%s'...\n", EntityPrm.GetName().CStr());
			if (!EntityDesc.IsValid() || !ProcessEntity(*EntityDesc))
			{
				n_msg(VL_ERROR, "Error processing entity '%s'\n", EntityPrm.GetName().CStr());
				FAIL;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessDescsInFolder(const CString& SrcPath, const CString& ExportPath)
{
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? SrcPath : ExportPath))
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		FAIL;
	}

	if (ExportDescs) IOSrv->CreateDirectory(ExportPath);

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			CString DescName = Browser.GetCurrEntryName();
			DescName.StripExtension();

			n_msg(VL_INFO, "Processing desc '%s'...\n", DescName.CStr());

			CString ExportFilePath = ExportPath + "/" + DescName + ".prm";
			if (!ProcessDesc(Browser.GetCurrentPath() + "/" + Browser.GetCurrEntryName(), ExportFilePath))
			{
				n_msg(VL_ERROR, "Error loading desc '%s'\n", DescName.CStr());
				continue;
			}
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessDescsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

bool ProcessQuestsInFolder(const CString& SrcPath, const CString& ExportPath)
{
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? SrcPath : ExportPath))
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		FAIL;
	}

	if (ExportDescs) IOSrv->CreateDirectory(ExportPath);

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			CString LowerName = Browser.GetCurrEntryName();
			LowerName.ToLower();

			if (LowerName != (ExportDescs ? "_quest.hrd" : "_quest.prm")) continue;

			CString QuestName;
			if (Verbose >= VL_INFO)
			{
				QuestName = Browser.GetCurrentPath();
				QuestName.ConvertBackslashes();
				QuestName.StripTrailingSlash();
				QuestName = QuestName.ExtractFileName();
			}

			n_msg(VL_INFO, "Processing quest '%s'...\n", QuestName.CStr());

			CString ExportFilePath = ExportPath + "/_Quest.prm";
			Data::PParams Desc;
			if (ExportDescs)
			{
				Desc = DataSrv->LoadHRD(SrcPath + "/_Quest.hrd", false);
				DataSrv->SavePRM(ExportFilePath, Desc);
			}
			else Desc = DataSrv->LoadPRM(ExportFilePath, false);

			if (!Desc.IsValid())
			{
				n_msg(VL_ERROR, "Error loading quest '%s' desc\n", QuestName.CStr());
				continue;
			}

			FilesToPack.InsertSorted(ExportFilePath);

			Data::PParams Tasks;
			if (Desc->Get(Tasks, CStrID("Tasks")))
			{
				for (int i = 0; i < Tasks->GetCount(); ++i)
				{
					CString Name = Tasks->Get(i).GetName().CStr();
					ExportFilePath = ExportPath + "/" + Name + ".lua";
					if (ExportDescs)
					{
						CString SrcFilePath = SrcPath + "/" + Name + ".lua";
						if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
						{
							BatchToolInOut(CStrID("CFLua"), SrcFilePath, ExportFilePath);
							FilesToPack.InsertSorted(ExportFilePath);
						}
					}
					else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);
				}
			}
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessQuestsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

bool ProcessSOActionTplsDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	for (int i = 0; i < Desc->GetCount(); ++i)
	{
		const Data::CParams& ActTpl = *Desc->Get<Data::PParams>(i);

		CString ScriptFile;
		if (ActTpl.Get<CString>(ScriptFile, CStrID("Script")))
		{
			CString ExportFilePath = "Scripts:" + ScriptFile + ".lua";
			if (!IsFileAdded(ExportFilePath))
			{
				if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScripts:" + ScriptFile + ".lua", ExportFilePath);
				FilesToPack.InsertSorted(ExportFilePath);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------
