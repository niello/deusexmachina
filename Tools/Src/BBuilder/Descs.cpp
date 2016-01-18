#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryWriter.h>
#include <IO/PathUtils.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

void ConvertPropNamesToFourCC(Data::PDataArray Props)
{
	for (int i = 0; i < Props->GetCount(); ++i)
	{
		if (!Props->Get(i).IsA<CString>()) continue;
		const CString& Name = Props->Get<CString>(i);
		Data::CFourCC Value;
		if (PropCodes.Get(Name, Value)) Props->At(i) = (int)Value.Code;
	}
}
//---------------------------------------------------------------------

bool ProcessDialogue(const CString& SrcContext, const CString& ExportContext, const CString& Name)
{
	CString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	bool ScriptExported = false;

	// Script can implements functions like OnStart(), so export anyway
	const CString& ScriptFile = Desc->Get<CString>(CStrID("Script"), CString::Empty);
	if (ScriptFile.IsValid())
	{
		ExportFilePath = CString("Scripts:") + ScriptFile + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), CString("SrcScripts:") + ScriptFile + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
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
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

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
				n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", PathUtils::GetExtension(CDLODFilePath));
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
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

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
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

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
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	CString BaseName = Desc->Get(CStrID("_Base_"), CString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDescWithParents(SrcContext, ExportContext, BaseName));
}
//---------------------------------------------------------------------

bool ProcessMaterialDesc(const char* pName)
{
	CString ExportFilePath("Materials:");
	ExportFilePath += pName;
	ExportFilePath += ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(CString("SrcMaterials:") + pName + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

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

			if (AttrDesc->Get(pValue, CStrID("SkinInfo")))
			{
				CString FileName = CString("Scene:") + pValue->GetValue<CStrID>().CStr() + ".skn";
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
						n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", PathUtils::GetExtension(CDLODFilePath));
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
	if (!Desc.IsValidPtr()) FAIL;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		IO::CFileStream File(ExportFilePath);
		if (!File.Open(IO::SAM_WRITE)) FAIL;
		IO::CBinaryWriter Writer(File);
		Writer.WriteParams(*Desc, *DataSrv->GetDataScheme(CStrID("SceneNode")));
		File.Close();
	}

	FilesToPack.InsertSorted(ExportFilePath);

	return ProcessSceneNodeRefs(*Desc);
}
//---------------------------------------------------------------------

bool ProcessDesc(const char* pSrcFilePath, const char* pExportFilePath)
{
	// Some descs can be loaded twice or more from different IO path assigns, avoid it
	CString RealExportFilePath = IOSrv->ResolveAssigns(pExportFilePath);

	if (IsFileAdded(RealExportFilePath)) OK;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(RealExportFilePath));
		if (!DataSrv->SavePRM(RealExportFilePath, DataSrv->LoadHRD(pSrcFilePath, false))) FAIL;
	}
	FilesToPack.InsertSorted(RealExportFilePath);

	OK;
}
//---------------------------------------------------------------------

//!!!TMP!
#include <Math/Matrix44.h>
#include <Math/TransformSRT.h>

//!!!TMP!
struct CBoneInfo
{
	matrix44	Pose;
	CStrID		BoneID;
	UPTR		ParentIndex;

	CBoneInfo(): ParentIndex(INVALID_INDEX) {}
};

//!!!TMP!
void GatherSkinInfo(CStrID NodeID, Data::PParams NodeDesc, CArray<CBoneInfo>& OutSkinInfo, UPTR ParentIndex)
{
	UPTR BoneIndex = ParentIndex;
	Data::PDataArray Attrs;
	if (NodeDesc->Get<Data::PDataArray>(Attrs, CStrID("Attrs")))
	{
		for (int i = 0; i < Attrs->GetCount(); ++i)
		{
			Data::PParams AttrDesc = Attrs->Get<Data::PParams>(i);
			CString ClassName;
			if (AttrDesc->Get<CString>(ClassName, CStrID("Class")) && ClassName == "Bone")
			{
				BoneIndex = (UPTR)AttrDesc->Get<int>(CStrID("BoneIndex"));
				CBoneInfo& BoneInfo = OutSkinInfo.At(BoneIndex);

				vector4 VRotate = AttrDesc->Get<vector4>(CStrID("PoseR"), vector4(0.f, 0.f, 0.f, 1.f));
				Math::CTransformSRT SRTPoseLocal;
				SRTPoseLocal.Scale = AttrDesc->Get<vector3>(CStrID("PoseS"), vector3(1.f, 1.f, 1.f));
				SRTPoseLocal.Translation = AttrDesc->Get<vector3>(CStrID("PoseT"), vector3::Zero);
				SRTPoseLocal.Rotation.set(VRotate.x, VRotate.y, VRotate.z, VRotate.w);

				SRTPoseLocal.ToMatrix(BoneInfo.Pose);	
				if (ParentIndex != INVALID_INDEX) BoneInfo.Pose.mult_simple(OutSkinInfo[ParentIndex].Pose);

				BoneInfo.BoneID = NodeID;
				BoneInfo.ParentIndex = ParentIndex;
			}
		}
	}

	Data::PParams Children;
	if (NodeDesc->Get<Data::PParams>(Children, CStrID("Children")))
		for (int i = 0; i < Children->GetCount(); ++i)
		{
			Data::CParam& Child = Children->Get(i);
			GatherSkinInfo(Child.GetName(), Child.GetValue<Data::PParams>(), OutSkinInfo, BoneIndex);
		}
}
//---------------------------------------------------------------------

//!!!TMP!
bool ConvertOldSkinInfo(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
	if (!Desc.IsValidPtr()) FAIL;

	CArray<CBoneInfo> SkinInfo;

	GatherSkinInfo(CStrID::Empty, Desc, SkinInfo, INVALID_INDEX);

	if (SkinInfo.GetCount())
	{
		IO::CFileStream File(ExportFilePath);
		if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) FAIL;
		IO::CBinaryWriter Writer(File);

		Writer.Write<U32>('SKIF');	// Magic
		Writer.Write<U32>(1);		// Format version

		Writer.Write<U32>(SkinInfo.GetCount());

		// Write pose array
		for (int i = 0; i < SkinInfo.GetCount(); ++i)
		{
			CBoneInfo& BoneInfo = SkinInfo[i];
			matrix44 Pose;
			BoneInfo.Pose.invert_simple(Pose);
			Writer.Write(Pose);
		}

		// Write bone hierarchy
		for (int i = 0; i < SkinInfo.GetCount(); ++i)
		{
			CBoneInfo& BoneInfo = SkinInfo[i];
			Writer.Write<U16>(BoneInfo.ParentIndex); // Invalid index is converted correctly, all bits set
			Writer.Write(BoneInfo.BoneID);
		}

		File.Close();

		FilesToPack.InsertSorted(ExportFilePath);
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	// Entity tpls aren't exported by ref here, because ALL that tpls are exported.
	// This allows to instantiate unreferenced tpls at the runtime.

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
		if (!ProcessDescWithParents(CString("SrcActors:"), CString("Actors:"), AttrValue))
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

		if (!ProcessSOActionTplsDesc(CString("SrcAI:AISOActionTpls.hrd"), CString("AI:AISOActionTpls.prm")))
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
		if (!ProcessDialogue(CString("SrcDlg:"), CString("Dlg:"), AttrValue))
		{
			n_msg(VL_ERROR, "Error processing dialogue desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("SceneFile")))
	{
		if (!ProcessSceneResource("SrcScene:" + AttrValue + ".hrd", "Scene:" + AttrValue + ".scn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource '%s'\n", AttrValue.CStr());
			FAIL;
		}

		//!!!TMP! Convert old skin info to new
		if (!ConvertOldSkinInfo("SrcScene:" + AttrValue + ".hrd", "Scene:" + AttrValue + ".skn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource skin info '%s'\n", AttrValue.CStr());
			FAIL;
		}
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
			if (!EntityDesc.IsValidPtr() || !ProcessEntity(*EntityDesc))
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
			CString DescName = PathUtils::ExtractFileNameWithoutExtension(Browser.GetCurrEntryName());

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

bool ProcessEntityTplsInFolder(const CString& SrcPath, const CString& ExportPath)
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
			CString DescName = PathUtils::ExtractFileNameWithoutExtension(Browser.GetCurrEntryName());

			n_msg(VL_INFO, "Processing entity tpl '%s'...\n", DescName.CStr());

			CString SrcFilePath = Browser.GetCurrentPath() + "/" + Browser.GetCurrEntryName();
			CString RealExportFilePath = IOSrv->ResolveAssigns(ExportPath + "/" + DescName + ".prm");

			if (IsFileAdded(RealExportFilePath)) continue;

			if (ExportDescs)
			{
				Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
				if (!Desc.IsValidPtr())
				{
					n_msg(VL_ERROR, "Error loading entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}

				Data::PDataArray Props;
				if (Desc->Get<Data::PDataArray>(Props, CStrID("Props")) && Props->GetCount())
					ConvertPropNamesToFourCC(Props);

				IOSrv->CreateDirectory(PathUtils::ExtractDirName(RealExportFilePath));
				if (!DataSrv->SavePRM(RealExportFilePath, Desc))
				{
					n_msg(VL_ERROR, "Error saving entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}

				if (!ProcessEntity(*Desc))
				{
					n_msg(VL_ERROR, "Error processing entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}
			}
			FilesToPack.InsertSorted(RealExportFilePath);
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessEntityTplsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
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
				QuestName.Trim(" \r\n\t\\/", false);
				QuestName = PathUtils::ExtractFileName(QuestName);
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

			if (!Desc.IsValidPtr())
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
					CString Name(Tasks->Get(i).GetName().CStr());
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
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

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
