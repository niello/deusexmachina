#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/materials --path Data ../../../content

class CMaterialTool : public CContentForgeTool
{
public:

	CMaterialTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".mtl");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read effect hrd

		Data::CParams Desc;
		{
			std::vector<char> In;
			if (!ReadAllFile(Task.SrcFilePath.string().c_str(), In, false))
			{
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " reading error");
				return false;
			}

			Data::CHRDParser Parser;
			if (!Parser.ParseBuffer(In.data(), In.size(), Desc))
			{
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD parsing error");
				return false;
			}
		}

		const std::string EffectID = GetParam<std::string>(Desc, "Effect", std::string{});
		if (EffectID.empty())
		{
			Task.Log.LogError("Material must reference an effect");
			return false;
		}

		auto ItParams = Desc.find(CStrID("Params"));
		if (ItParams != Desc.cend())
		{
			if (!ItParams->second.IsA<Data::CParams>())
			{
				Task.Log.LogError("'RenderStates' must be a section");
				return false;
			}

			// Get material table from the effect file

			auto Path = ResolvePathAliases(EffectID);
			Task.Log.LogDebug("Opening effect " + Path.generic_string());

			auto FilePtr = std::make_shared<std::ifstream>(Path, std::ios_base::binary);
			auto& File = *FilePtr;
			if (!File)
			{
				Task.Log.LogError("Can't open effect " + Path.generic_string());
				return false;
			}

			if (ReadStream<uint32_t>(File) != 'SHFX')
			{
				Task.Log.LogError("Wrong effect file format in " + Path.generic_string());
				return false;
			}

			if (ReadStream<uint32_t>(File) > 0x00010000)
			{
				Task.Log.LogError("Unsupported effect version in " + Path.generic_string());
				return false;
			}

			// Skip material type
			ReadStream<std::string>(File);

			// Offset is the start of the global table of corresponding metadata, because
			// global table is always conveniently placed at the metadata start.
			const uint32_t FormatCount = ReadStream<uint32_t>(File);
			for (size_t i = 0; i < FormatCount; ++i)
			{
				const auto ShaderFormat = ReadStream<uint32_t>(File);
				const auto Offset = ReadStream<uint32_t>(File);

				//auto It = Globals.find(ShaderFormat);
				//if (It == Globals.end())
				//	It = Globals.emplace(ShaderFormat, CGlobalTable{}).first;

				//It->second.Sources.emplace_back(CGlobalTable::CSource{ FilePtr, Offset });
			}

			// Process parameter values

			auto& ParamDescs = ItParams->second.GetValue<Data::CParams>();
			for (const auto& ParamDesc : ParamDescs)
			{
				Task.Log.LogDebug("Material param: " + ParamDesc.first.ToString());
			
				// if ID found in constants
				// else if in resources
				// else if in samplers
			}
		}

		// Write resulting file

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary);
		if (!File)
		{
			Task.Log.LogError("Error opening an output file");
			return false;
		}

		WriteStream<uint32_t>(File, 'MTRL');     // Format magic value
		WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0
		WriteStream(File, EffectID);

		return true;
	}
};

int main(int argc, const char** argv)
{
	CMaterialTool Tool("cf-material", "DeusExMachina material compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
