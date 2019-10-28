#include <ContentForgeTool.h>
#include <Render/SM30ShaderMeta.h>
#include <Render/USMShaderMeta.h>
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

		CMaterialParams MaterialParams;

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

			std::map<uint32_t, uint32_t> MaterialTableOffsets;
			const uint32_t FormatCount = ReadStream<uint32_t>(File);
			for (size_t i = 0; i < FormatCount; ++i)
			{
				const auto ShaderFormat = ReadStream<uint32_t>(File);
				const auto Offset = ReadStream<uint32_t>(File);
				MaterialTableOffsets.emplace(ShaderFormat, Offset);
			}

			// Process material tables for all formats, build cross-format table

			for (auto& Pair : MaterialTableOffsets)
			{
				File.seekg(Pair.second, std::ios_base::beg);

				// Get offset to material param table (the end of the global table)
				// Skip to the material table start (skip uint32_t table size too)
				const auto Offset = ReadStream<uint32_t>(File);
				File.seekg(Offset + sizeof(uint32_t), std::ios_base::cur);

				switch (Pair.first)
				{
					case 'DX9C':
					{
						CSM30EffectMeta Meta;
						File >> Meta;
						if (!CollectMaterialParams(MaterialParams, Meta))
						{
							Task.Log.LogError("Material metadata is incompatible across different shader formats");
							return false;
						}
						break;
					}
					case 'DXBC':
					{
						CUSMEffectMeta Meta;
						File >> Meta;
						if (!CollectMaterialParams(MaterialParams, Meta))
						{
							Task.Log.LogError("Material metadata is incompatible across different shader formats");
							return false;
						}
						break;
					}
					default:
					{
						Task.Log.LogWarning("Skipping unsupported shader format: " + FourCC(Pair.first));
						continue;
					}
				}
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

		// Serialize values
		if (ItParams != Desc.cend())
		{
			const auto& ParamDescs = ItParams->second.GetValue<Data::CParams>();
			if (!WriteMaterialParams(File, MaterialParams, ParamDescs, Task.Log))
			{
				Task.Log.LogError("Error serializing values");
				return false;
			}
		}
		else WriteStream<uint32_t>(File, 0); // Value count

		return true;
	}
};

int main(int argc, const char** argv)
{
	CMaterialTool Tool("cf-material", "DeusExMachina material compiler", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
