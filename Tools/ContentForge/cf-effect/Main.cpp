#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>
//#include <CLI11.hpp>
#include <thread>
//#include <mutex>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/effects

struct CRenderState
{
	// pipeline settings

	// shader data

	uint32_t ShaderFormatFourCC = 0;
	uint32_t MinFeatureLevel = 0;
	uint32_t RequiresFlags = 0;
	int8_t LightCount = -1;
	bool IsValid = false;
};

class CEffectTool : public CContentForgeTool
{
private:

	// FIXME: common threadsafe logger for tasks instead of cout
	//std::mutex COutMutex;

public:

	CEffectTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual int Init() override
	{
		return 0;
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		return false;
	}

	//virtual void ProcessCommandLine(CLI::App& CLIApp) override
	//{
	//	CContentForgeTool::ProcessCommandLine(CLIApp);
	//	CLIApp.add_option("--db", _DBPath, "Shader DB file path");
	//	CLIApp.add_option("--is,--inputsig", _InputSignaturesDir, "Folder where input signature binaries will be saved");
	//}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		const auto DestPath = fs::path(Output) / (TaskID + ".eff");

		// FIXME: must be thread-safe, also can move to the common code
		const auto LineEnd = std::cout.widen('\n');
		if (_LogVerbosity >= EVerbosity::Debug)
		{
			std::cout << "Source: " << Task.SrcFilePath.generic_string() << LineEnd;
			std::cout << "Task: " << Task.TaskID.CStr() << LineEnd;
			std::cout << "Thread: " << std::this_thread::get_id() << LineEnd;
		}

		// Read effect hrd

		Data::CParams Desc;
		{
			std::vector<char> In;
			if (!ReadAllFile(Task.SrcFilePath.string().c_str(), In, false))
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << Task.SrcFilePath.generic_string() << " reading error" << LineEnd;
				return false;
			}

			Data::CHRDParser Parser;
			if (!Parser.ParseBuffer(In.data(), In.size(), Desc))
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << Task.SrcFilePath.generic_string() << " HRD parsing error" << LineEnd;
				return false;
			}
		}

		// Get and validate material type

		const CStrID MaterialType = GetParam<CStrID>(Desc, "Type", CStrID::Empty);
		if (!MaterialType)
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "Material type not specified" << LineEnd;
			return false;
		}

		// Get and validate global and material params

		auto ItGlobalParams = Desc.find(CStrID("GlobalParams"));
		const bool HasGlobalParams = (ItGlobalParams != Desc.cend() && !ItGlobalParams->second.IsVoid());
		if (HasGlobalParams && !ItGlobalParams->second.IsA<Data::CDataArray>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "'GlobalParams' must be an array of CStringID" << LineEnd;
			return false;
		}

		auto ItMaterialParams = Desc.find(CStrID("MaterialParams"));
		const bool HasMaterialParams = (ItMaterialParams != Desc.cend() && !ItMaterialParams->second.IsVoid());
		if (HasMaterialParams && !ItMaterialParams->second.IsA<Data::CParams>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "'MaterialParams' must be a section" << LineEnd;
			return false;
		}

		if (HasGlobalParams && HasMaterialParams)
		{
			// Check intersections between global and material params
			bool HasErrors = false;
			const Data::CDataArray& GlobalParams = ItGlobalParams->second.GetValue<Data::CDataArray>();
			const Data::CParams& MaterialParams = ItMaterialParams->second.GetValue<Data::CParams>();
			for (const auto& Global : GlobalParams)
			{
				if (!Global.IsA<CStrID>())
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << "'GlobalParams' can contain only CStringID elements (single-quoted strings)" << LineEnd;
					HasErrors = true;
				}

				const CStrID ParamID = Global.GetValue<CStrID>();
				if (MaterialParams.find(ParamID) != MaterialParams.cend())
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << '\'' << ParamID.CStr() << "' appears in both global and material params" << LineEnd;
					HasErrors = true;
				}
			}
		}

		// Parse, validate and sort techniques

		auto ItRenderStates = Desc.find(CStrID("RenderStates"));
		if (ItRenderStates == Desc.cend() || !ItRenderStates->second.IsA<Data::CParams>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "'RenderStates' must be a section" << LineEnd;
			return false;
		}

		auto ItTechs = Desc.find(CStrID("Techniques"));
		if (ItTechs == Desc.cend() || !ItTechs->second.IsA<Data::CParams>() || ItTechs->second.GetValue<Data::CParams>().empty())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "'Techniques' must be a section and contain at least one input set" << LineEnd;
			return false;
		}

		std::map<CStrID, CRenderState> RSCache;

		const auto& RenderStates = ItRenderStates->second.GetValue<Data::CParams>();
		const auto& InputSets = ItTechs->second.GetValue<Data::CParams>();
		for (const auto& InputSet : InputSets)
		{
			if (!InputSet.second.IsA<Data::CParams>() || InputSet.second.GetValue<Data::CParams>().empty())
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Input set '" << InputSet.first.CStr() << "' must be a section containing at least one technique" << LineEnd;
				return false;
			}

			const auto& Techs = InputSet.second.GetValue<Data::CParams>();
			for (const auto& Tech : Techs)
			{
				if (!Tech.second.IsA<Data::CDataArray>() || Tech.second.GetValue<Data::CDataArray>().empty())
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << "Tech '" << InputSet.first.CStr() << '.' << Tech.first.CStr() << "' must be an array containing at least one render state ID" << LineEnd;
					return false;
				}

				// TODO:
				// parse passes //???before techs to handle references properly?
				// if tech mixes shader formats, fail (or allow say DXBC+DXIL for D3D12? use newer format as tech format? DXIL in this case)
				// if tech became empty, discard it
				uint32_t ShaderFormatFourCC = 0;
				uint32_t MinFeatureLevel = 0;
				uint32_t RequiresFlags = 0;

				const auto& Passes = Tech.second.GetValue<Data::CDataArray>();
				for (const auto& Pass : Passes)
				{
					const CStrID RenderStateID = Pass.GetValue<CStrID>();

					if (_LogVerbosity >= EVerbosity::Debug)
						std::cout << "Tech '" << InputSet.first.CStr() << '.' << Tech.first.CStr() << "', pass '" << RenderStateID.CStr() << '\'' << LineEnd;

					auto ItRS = RSCache.find(RenderStateID);
					if (ItRS == RSCache.cend())
					{
						LoadRenderState(RSCache, RenderStateID, RenderStates);
						auto ItRS = RSCache.find(RenderStateID);
					}

					const auto& RS = ItRS->second;
					if (!RS.IsValid)
					{
						// Can discard only this tech, but for now issue an error and stop
						if (_LogVerbosity >= EVerbosity::Errors)
							std::cout << "Tech '" << InputSet.first.CStr() << '.' << Tech.first.CStr() << "' uses invalid render state in a pass '" << RenderStateID.CStr() << '\'' << LineEnd;
						return false;
					}

					// collect info from the render state:
					// shader format
					// feature level
					// light count
				}
			}

			// All techs of the input set:
			// sort techs by the feature level (descending), so that first loaded tech is the best
			//???sort by shader format?
		}

		// FIXME: must be thread-safe, also can move to the common code
		//if (_LogVerbosity >= EVerbosity::Debug)
		//	std::cout << "Status: " << (Ok ? "OK" : "FAIL") << LineEnd << LineEnd;

		return true;
	}

	bool LoadRenderState(std::map<CStrID, CRenderState>& RSCache, CStrID ID, const Data::CParams& RenderStates)
	{
		// Insert invalid render state initially not to do it in every 'return false' below.
		// We want to have a record for each parsed RS, not only for valid ones.
		CRenderState& RS = RSCache.emplace(ID, CRenderState{}).first->second;

		const auto LineEnd = std::cout.widen('\n');

		// Get a descripting HRD section

		auto It = RenderStates.find(ID);
		if (It == RenderStates.cend() || !It->second.IsA<Data::CParams>)
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "Render state '" << ID.CStr() << "' not found or is not a section" << LineEnd;
			return false;
		}

		const auto& Desc = It->second.GetValue<Data::CParams>();

		// Load base state if specified

		const CStrID BaseID = GetParam(Desc, "Base", CStrID::Empty);
		if (BaseID)
		{
			auto ItRS = RSCache.find(BaseID);
			if (ItRS == RSCache.cend())
			{
				LoadRenderState(RSCache, BaseID, RenderStates);
				auto ItRS = RSCache.find(BaseID);
			}

			const auto& BaseRS = ItRS->second;
			if (!BaseRS.IsValid)
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Render state '" << ID.CStr() << "' has invalid base state '" << BaseID.CStr() << '\'' << LineEnd;
				return false;
			}

			// Copy base state to the current, so all parameters in the current desc will override base values
			RS = BaseRS;
		}

		// Load explicit state

		//...

		// Process shaders

		// ...

		// store only explicitly defined values? easier loading, engine supplies defaults

		// loop through specified shaders
		// - if no shader resource, fail
		// - if unsupported format mixing, fail
		// - load metadata (separate codepath for each metadata format)

		// get shader formats for all specified shaders
		// if shader doesn't exist, fail
		// if pass mixes shader formats, fail (or allow say DXBC+DXIL for D3D12? use newer format as tech format? DXIL in this case)
		// collect feature level (max through all used shaders) - only for DX? additional fields can be based on tech format
		// get max light count of all shaders in all passes (-1 = any, 0 = no lights in this pass)
		// shader switching is costly, so we don't build per-light-count variations, but instead
		// create the shader with max light count only
		//???how to claculate max light count and whether the shader uses lights at all?

		RSCache.emplace(ID, std::move(RS));
		RS.IsValid = true;
		return true;
	}
};

int main(int argc, const char** argv)
{
	CEffectTool Tool("cf-effect", "DeusExMachina rendering effect compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
