#include "Main.h"

#include <IO/IOServer.h>
#include <IO/PathUtils.h>
#include <Data/DataServer.h>
#include <DEMShaderCompilerDLL.h>

//???get ID & dest from desc or from here? use some parameter in desc pathes?
bool ProcessResourceDesc(const CString& RsrcFileName, const CString& ExportFileName)
{
	//!!!check if aready was exported!

	Data::PParams Desc = DataSrv->LoadHRD(RsrcFileName, false);
	if (!Desc.IsValidPtr()) FAIL;

	CString RsrcDir = PathUtils::ExtractDirName(RsrcFileName);

	for (UPTR i = 0; i < Desc->GetCount(); ++i)
	{
		Data::PParams RsrcDesc = Desc->Get<Data::PParams>(i);

		n_msg(VL_DETAILS, "  Processing CFD resource %s.%s...\n", RsrcFileName.CStr(), Desc->Get(i).GetName().CStr());

		CStrID Tool = RsrcDesc->Get<CStrID>(CStrID("Tool"));

		if (Tool == CStrID("CFTerrain"))
		{
			CString InStr = IOSrv->ResolveAssigns(RsrcDir + RsrcDesc->Get<CString>(CStrID("In")));
			CString OutStr = IOSrv->ResolveAssigns(ExportFileName);

			int PatchSize = RsrcDesc->Get<int>(CStrID("PatchSize"), 8);
			int LODCount = RsrcDesc->Get<int>(CStrID("LODCount"), 6);

			if (InStr.FindIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
			if (OutStr.FindIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

			CString WorkingDir;
			Sys::GetWorkingDirectory(WorkingDir);

			char CmdLine[MAX_CMDLINE_CHARS];
			sprintf_s(CmdLine, "-v %d -patch %d -lod %d -in %s -out %s",
				ExternalVerbosity, PatchSize, LODCount, InStr.CStr(), OutStr.CStr());
			int ExitCode = RunExternalToolAsProcess(Tool, CmdLine, WorkingDir.CStr());
			if (ExitCode != 0)
			{
				n_msg(VL_ERROR, "External tool %s execution failed\n", Tool.CStr());
				FAIL;
			}
		}
		else if (Tool == CStrID("CFCopy") || Tool == CStrID("CFLua"))
		{
			BatchToolInOut(Tool, RsrcDir + RsrcDesc->Get<CString>(CStrID("In")), RsrcDesc->Get(CStrID("Out"), ExportFileName));
		}
		else FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

//???does CFShader really need ProjectDir? why other tools don't? mb pass shader DB path and shader compiler DLL instead?
//???cut out IOSrv assigns from it and use only absolute pathes?
bool ExportEffect(const CString& SrcFilePath, const CString& ExportFilePath, bool LegacySM30)
{
	CString InStr = IOSrv->ResolveAssigns(SrcFilePath);
	CString OutStr = IOSrv->ResolveAssigns(ExportFilePath);

	if (InStr.FindIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
	if (OutStr.FindIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

	CString WorkingDir;
	Sys::GetWorkingDirectory(WorkingDir);

	char CmdLine[MAX_CMDLINE_CHARS];
	sprintf_s(CmdLine, "-v %d %s -proj %s -in %s -out %s",
		ExternalVerbosity, LegacySM30 ? "-sm3" : "", ProjectDir.CStr(), InStr.CStr(), OutStr.CStr());
	int ExitCode = RunExternalToolAsProcess(CStrID("CFShader"), CmdLine, WorkingDir.CStr());
	if (ExitCode != 0)
	{
		n_msg(VL_ERROR, "External tool CFShader execution failed\n");
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessFrameShader(const Data::CParams& Desc)
{
	Data::PParams ShaderList;
	if (Desc.Get(ShaderList, CStrID("Shaders")))
	{
		for (UPTR i = 0; i < ShaderList->GetCount(); ++i)
		{
			CString ExportFilePath = "Shaders:" + ShaderList->Get<CString>(i) + ".fxo";

			if (IsFileAdded(ExportFilePath)) continue;

			if (ExportDescs)
				BatchToolInOut(CStrID("CFShader"), "SrcShaders:" + ShaderList->Get<CString>(i) + ".fx", ExportFilePath);

			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	//!!!Parse for textures!

	OK;
}
//---------------------------------------------------------------------

bool ProcessShaderResourceDesc(const Data::CParams& Desc, bool Debug, U32& OutShaderID)
{
	CString SrcPath;
	Desc.Get(SrcPath, CStrID("In"));
	CStrID Type = Desc.Get(CStrID("Type"), CStrID::Empty);
	CString EntryPoint;
	Desc.Get(EntryPoint, CStrID("Entry"));
	int Target = 0;
	Desc.Get(Target, CStrID("Target"));

	if (!IncludeSM30ShadersAndEffects && ((U32)Target < 0x0400))
	{
		OutShaderID = 0;
		OK;
	}
	
	CString Defines;
	if (Desc.Get(Defines, CStrID("Defines"))) Defines.Trim();

	EShaderType ShaderType;
	if (Type == CStrID("Vertex")) ShaderType = ShaderType_Vertex;
	else if (Type == CStrID("Pixel")) ShaderType = ShaderType_Pixel;
	else if (Type == CStrID("Geometry")) ShaderType = ShaderType_Geometry;
	else if (Type == CStrID("Hull")) ShaderType = ShaderType_Hull;
	else if (Type == CStrID("Domain")) ShaderType = ShaderType_Domain;
	else FAIL;

	CString WorkingDir;
	Sys::GetWorkingDirectory(WorkingDir);
	SrcPath = PathUtils::GetAbsolutePath(WorkingDir, IOSrv->ResolveAssigns(SrcPath));

	U32 ObjID, SigID;
	int Result = DLLCompileShader(SrcPath.CStr(), ShaderType, (U32)Target, EntryPoint.CStr(), Defines.CStr(), Debug, ObjID, SigID);

	if (Result == DEM_SHADER_COMPILER_SUCCESS)
	{
		OutShaderID = ObjID;
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------
