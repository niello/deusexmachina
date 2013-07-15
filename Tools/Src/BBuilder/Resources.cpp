#include "Main.h"

#include <IO/IOServer.h>
#include <Data/DataServer.h>

//???get ID & dest from desc or from here? use some parameter in desc pathes?
bool ProcessResourceDesc(const CString& RsrcFileName, const CString& ExportFileName)
{
	//!!!check if aready was exported!

	Data::PParams Desc = DataSrv->LoadHRD(RsrcFileName, false);
	if (!Desc.IsValid()) FAIL;

	CString RsrcDir = RsrcFileName.ExtractDirName();

	for (int i = 0; i < Desc->GetCount(); ++i)
	{
		Data::PParams RsrcDesc = Desc->Get<Data::PParams>(i);

		n_msg(VL_DETAILS, "  Processing CFD resource %s.%s...\n", RsrcFileName.CStr(), Desc->Get(i).GetName().CStr());

		CStrID Tool = RsrcDesc->Get<CStrID>(CStrID("Tool"));

		if (Tool == CStrID("CFTerrain"))
		{
			CString InStr = IOSrv->ManglePath(RsrcDir + RsrcDesc->Get<CString>(CStrID("In")));
			CString OutStr = IOSrv->ManglePath(ExportFileName);

			int PatchSize = RsrcDesc->Get<int>(CStrID("PatchSize"), 8);
			int LODCount = RsrcDesc->Get<int>(CStrID("LODCount"), 6);

			if (InStr.FindCharIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
			if (OutStr.FindCharIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

			char CmdLine[MAX_CMDLINE_CHARS];
			sprintf_s(CmdLine, "-v %d -patch %d -lod %d -in %s -out %s",
				ExternalVerbosity, PatchSize, LODCount, InStr.CStr(), OutStr.CStr());
			int ExitCode = RunExternalToolAsProcess(Tool, CmdLine);
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

bool ProcessFrameShader(const Data::CParams& Desc)
{
	Data::PParams ShaderList;
	if (Desc.Get(ShaderList, CStrID("Shaders")))
	{
		for (int i = 0; i < ShaderList->GetCount(); ++i)
		{
			CString ExportFilePath = "Shaders:" + ShaderList->Get<CString>(i) + ".fxo";

			if (IsFileAdded(ExportFilePath)) continue;

			if (ExportShaders)
				BatchToolInOut(CStrID("CFShader"), "SrcShaders:" + ShaderList->Get<CString>(i) + ".fx", ExportFilePath);

			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	//!!!Parse for textures!

	OK;
}
//---------------------------------------------------------------------
