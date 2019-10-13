#include <CFRenderPathFwd.h>
#include <ShaderMeta/USMShaderMeta.h>
#include <Logging.h>
#include <sstream>

bool BuildGlobalsTableForDXBC(CGlobalTable& Task, CThreadSafeLog* pLog)
{
	CUSMEffectMeta RPGlobalMeta;
	RPGlobalMeta.PrintableName = "RP Global";

	// Load metadata from all effects, merge and verify compatibility

	for (const auto& Src : Task.Sources)
	{
		auto& InStream = *Src.Stream;

		InStream.seekg(Src.Offset, std::ios_base::beg);

		CUSMEffectMeta GlobalMeta, MaterialMeta;
		InStream >> GlobalMeta;
		InStream >> MaterialMeta;

		//
		int DBG_TMP = 0;
	}

	std::ostringstream OutStream(std::ios_base::binary);
	OutStream << RPGlobalMeta;
	Task.Result = OutStream.str();

	return true;
}
//---------------------------------------------------------------------
