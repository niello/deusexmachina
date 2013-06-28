#include "Main.h"

#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryWriter.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

bool ProcessDialogue(const nString& SrcContext, const nString& ExportContext, const nString& Name)
{
	nString ExportFilePath = ExportContext + Name + ".prm";

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

	bool UsesScript = false;

	const Data::CDataArray& Links = *(Desc->Get<Data::PDataArray>(CStrID("Links")));
	for (int j = 0; j < Links.GetCount(); j++)
	{
		//const Data::CDataArray& Link = *(Links[j]); // Crashes vs2008 compiler :)
		const Data::CDataArray& Link = *(Links.Get(j).GetValue<Data::PDataArray>());

		if (Link.GetCount() > 2)
		{
			if (Link.Get(2).GetValue<nString>().IsValid()) UsesScript = true;
			else if (Link.GetCount() > 3 && Link.Get(3).GetValue<nString>().IsValid()) UsesScript = true;
			if (UsesScript)
			{
				int Idx = Desc->IndexOf(CStrID("ScriptFile"));
				nString ScriptFile = (Idx == INVALID_INDEX) ? Name : Desc->Get(Idx).GetValue<nString>();

				ExportFilePath = ExportContext + ScriptFile + ".lua";

				if (!IsFileAdded(ExportFilePath))
				{
					if (ExportDescs) BatchToolInOut(CStrID("CFLua"), SrcContext + ScriptFile + ".lua", ExportFilePath);
					FilesToPack.InsertSorted(ExportFilePath);
				}

				break;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessCollisionShape(const nString& SrcFilePath, const nString& ExportFilePath)
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
	nString CDLODFile = Desc->Get(CStrID("CDLODFile"), nString::Empty);
	if (CDLODFile.IsValid())
	{
		nString CDLODFilePath = "Export:Terrain/" + CDLODFile + ".cdlod";
		if (!IsFileAdded(CDLODFilePath))
		{
			if (ExportResources &&
				!ProcessResourceDesc("Src:Terrain/" + CDLODFile + ".cfd", CDLODFilePath) &&
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

bool ProcessPhysicsDesc(const nString& SrcFilePath, const nString& ExportFilePath)
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
				if (!ProcessCollisionShape(	nString("Src:Physics/") + PickShape.CStr() + ".hrd",
											nString("Export:Physics/") + PickShape.CStr() + ".prm"))
				{
					n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
					FAIL;
				}
		}
	}


	OK;
}
//---------------------------------------------------------------------

bool ProcessAnimDesc(const nString& SrcFilePath, const nString& ExportFilePath)
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
		nString FileName = nString("Export:Anims/") + Desc->Get(i).GetValue<CStrID>().CStr();
		if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
	}

	OK;
}
//---------------------------------------------------------------------

//???pre-unwind descs on exports?
bool ProcessDescWithParents(const nString& SrcContext, const nString& ExportContext, const nString& Name)
{
	nString ExportFilePath = ExportContext + Name + ".prm";

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

	nString BaseName = Desc->Get(CStrID("_Base_"), nString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDescWithParents(SrcContext, ExportContext, BaseName));
}
//---------------------------------------------------------------------

bool ProcessMaterialDesc(const nString& Name)
{
	nString ExportFilePath = "Export:Materials/" + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD("Src:Materials/" + Name + ".hrd", false);
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
			nString FileName = nString("Export:Textures/") + Textures->Get(i).GetValue<CStrID>().CStr();
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
					nString FileName = nString("Export:Textures/") + Textures->Get(i).GetValue<CStrID>().CStr();
					if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
				}
			}

			Data::CData* pValue;

			if (AttrDesc->Get(pValue, CStrID("Material")))
				if (!ProcessMaterialDesc(pValue->GetValue<CStrID>().CStr())) FAIL;

			//!!!when export from src, find resource desc and add source Model to CFModel list!
			if (AttrDesc->Get(pValue, CStrID("Mesh")))
			{
				nString FileName = nString("Export:Meshes/") + pValue->GetValue<CStrID>().CStr() + ".nvx2"; //!!!change extension!
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}

			//!!!when export from src, find resource desc and add source BT to CFTerrain list!
			if (AttrDesc->Get(pValue, CStrID("CDLODFile")))
			{
				nString CDLODFilePath = "Export:Terrain/" + pValue->GetValue<nString>() + ".cdlod";
				if (!IsFileAdded(CDLODFilePath))
				{
					if (ExportResources &&
						!ProcessResourceDesc("Src:Terrain/" + pValue->GetValue<nString>() + ".cfd", CDLODFilePath) &&
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
bool ProcessSceneResource(const nString& SrcFilePath, const nString& ExportFilePath)
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

bool ProcessDesc(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		if (!DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD(SrcFilePath, false))) FAIL;
	}
	FilesToPack.InsertSorted(ExportFilePath);

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	nString AttrValue;

	if (Attrs->Get<nString>(AttrValue, CStrID("UIDesc")))
		if (!ProcessDesc("Src:Game/UI/" + AttrValue + ".hrd", "Export:Game/UI/" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing UI desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("ActorDesc")))
	{
		if (!ProcessDescWithParents("Src:Game/AI/Actors/", "Export:Game/AI/Actors/", AttrValue))
		{
			n_msg(VL_ERROR, "Error processing AI actor desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessDesc("Src:Game/AI/AIActionTpls.hrd", "Export:Game/AI/AIActionTpls.prm"))
		{
			n_msg(VL_ERROR, "Error processing shared AI action templates desc\n");
			FAIL;
		}
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("AIHintsDesc")))
		if (!ProcessDesc("Src:Game/AI/Hints/" + AttrValue + ".hrd", "Export:Game/AI/Hints/" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI hints desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("SmartObjDesc")))
	{
		if (!ProcessDesc("Src:Game/AI/Smarts/" + AttrValue + ".hrd", "Export:Game/AI/Smarts/" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI smart object desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessDesc("Src:Game/AI/AISOActionTpls.hrd", "Export:Game/AI/AISOActionTpls.prm"))
		{
			n_msg(VL_ERROR, "Error processing shared AI smart object action templates desc\n");
			FAIL;
		}
	}

	CStrID ItemID = Attrs->Get<CStrID>(CStrID("ItemTplID"), CStrID::Empty);
	if (ItemID.IsValid())
		if (!ProcessDesc(nString("Src:Game/Items/") + ItemID.CStr() + ".hrd", nString("Export:Game/Items/") + ItemID.CStr() + ".prm"))
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
				!ProcessDesc(	nString("Src:Game/Items/") + ItemID.CStr() + ".hrd",
								nString("Export:Game/Items/") + ItemID.CStr() + ".prm"))
			{
				n_msg(VL_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
				FAIL;
			}
		}
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("ScriptClass")))
	{
		nString ExportFilePath = "Export:Game/ScriptClasses/" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "Src:Game/ScriptClasses/" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("Script")))
	{
		nString ExportFilePath = "Export:Game/Scripts/" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "Src:Game/Scripts/" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	CStrID PickShape = Attrs->Get<CStrID>(CStrID("PickShape"), CStrID::Empty);
	if (PickShape.IsValid())
		if (!ProcessCollisionShape(	nString("Src:Physics/") + PickShape.CStr() + ".hrd",
									nString("Export:Physics/") + PickShape.CStr() + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("Physics")))
		if (!ProcessPhysicsDesc(nString("Src:Physics/") + AttrValue + ".hrd",
								nString("Export:Physics/") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing physics desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("AnimDesc")))
		if (!ProcessAnimDesc(	nString("Src:Game/Anim/") + AttrValue + ".hrd",
								nString("Export:Game/Anim/") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing animation desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("Dialogue")))
		if (!ProcessDialogue("Src:Game/Dlg/", "Export:Game/Dlg/", AttrValue))
		{
			n_msg(VL_ERROR, "Error processing dialogue desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("SceneFile")))
		if (!ProcessSceneResource("Src:Scene/" + AttrValue + ".hrd", "Export:Scene/" + AttrValue + ".scn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource '%s'\n", AttrValue.CStr());
			FAIL;
		}

	OK;
}
//---------------------------------------------------------------------

bool ProcessLevel(const Data::CParams& LevelDesc, const nString& Name)
{
	// Add level script

	nString ExportFilePath = "Export:Game/Levels/" + Name + ".lua";
	if (ExportDescs)
	{
		nString SrcFilePath = "Src:Game/Levels/" + Name + ".lua";
		if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
		{
			BatchToolInOut(CStrID("CFLua"), SrcFilePath, ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath); //???or after running tool?
		}
	}
	else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Add navmesh (always exported, no src)

	ExportFilePath = "Export:Game/Levels/" + Name + ".nm";
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
			n_msg(VL_INFO, "Processing entity '%s'...\n", EntityPrm.GetName().CStr());
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
