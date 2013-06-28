#include "Main.h"

#include <IO/IOServer.h>
#include <Data/DataServer.h>

//???get ID & dest from desc or from here? use some parameter in desc pathes?
bool ProcessResourceDesc(const nString& RsrcFileName, const nString& ExportFileName)
{
	//!!!check if aready was exported!

	Data::PParams Desc = DataSrv->LoadHRD(RsrcFileName, false);
	if (!Desc.IsValid()) FAIL;

	nString RsrcDir = RsrcFileName.ExtractDirName();

	for (int i = 0; i < Desc->GetCount(); ++i)
	{
		Data::PParams RsrcDesc = Desc->Get<Data::PParams>(i);

		CStrID Tool = RsrcDesc->Get<CStrID>(CStrID("Tool"));

		if (Tool == CStrID("CFTerrain"))
		{
			nString InStr = IOSrv->ManglePath(RsrcDir + RsrcDesc->Get<nString>(CStrID("In")));
			nString OutStr = IOSrv->ManglePath(ExportFileName);

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
			BatchToolInOut(Tool, RsrcDir + RsrcDesc->Get<nString>(CStrID("In")), RsrcDesc->Get(CStrID("Out"), ExportFileName));
		}
		else FAIL;
	}

	OK;
}
//---------------------------------------------------------------------
