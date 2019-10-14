#include <CFRenderPathFwd.h>
#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>
#include <thread>
#include <iostream>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/rpaths --path Data ../../../content

//???skip loading global metadata from effect when creating rpath in DEM? all relevant metadata is already copied to the rpath.
//???save global table to a separate file, not to the effect itself?
//???the same for shader metadata?

bool BuildGlobalsTableForDX9C(CGlobalTable& Task, CThreadSafeLog* pLog);
bool BuildGlobalsTableForDXBC(CGlobalTable& Task, CThreadSafeLog* pLog);

class CRenderPathTool : public CContentForgeTool
{
private:

public:

	CRenderPathTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: CStrID
		return false;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".rp");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read render path hrd

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

		// Build and verify global parameter table

		std::map<uint32_t, CGlobalTable> Globals;
		auto ItEffects = Desc.find(CStrID("Effects"));
		if (ItEffects != Desc.cend())
		{
			if (!ItEffects->second.IsA<Data::CDataArray>())
			{
				Task.Log.LogError("'Phases' must be an array of pathes to .eff files");
				return false;
			}

			// Collect global metadata sources

			const auto& EffectPathes = ItEffects->second.GetValue<Data::CDataArray>();
			for (const auto& EffectPathData : EffectPathes)
			{
				if (!EffectPathData.IsA<std::string>())
				{
					Task.Log.LogError("Wrong data in 'Effects' array, all elements must be strings");
					return false;
				}

				auto Path = ResolvePathAliases(EffectPathData.GetValue<std::string>());
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

					auto It = Globals.find(ShaderFormat);
					if (It == Globals.end())
						It = Globals.emplace(ShaderFormat, CGlobalTable{}).first;

					It->second.Sources.emplace_back(CGlobalTable::CSource{ FilePtr, Offset });
				}
			}

			// Process and verify globals from all effects

			for (auto& FormatToGlobals : Globals)
			{
				switch (FormatToGlobals.first)
				{
					case 'DX9C':
					{
						if (!BuildGlobalsTableForDX9C(FormatToGlobals.second, &Task.Log)) return false;
						break;
					}
					case 'DXBC':
					{
						if (!BuildGlobalsTableForDXBC(FormatToGlobals.second, &Task.Log)) return false;
						break;
					}
					default:
					{
						Task.Log.LogWarning("Skipping unsupported shader format: " + FourCC(FormatToGlobals.first));
						continue;
					}
				}
			}

			// Validate results

			for (auto ItGlobal = Globals.begin(); ItGlobal != Globals.end(); /**/)
			{
				if (ItGlobal->second.Result.empty())
				{
					Task.Log.LogWarning("No data serialized for the format: " + FourCC(ItGlobal->first));
					ItGlobal = Globals.erase(ItGlobal);
				}
				else if (ItGlobal->second.Result.size() > std::numeric_limits<uint32_t>().max())
				{
					// We don't support 64-bit offsets in render path files. If your data is so big,
					// most probably it is a serialization error.
					Task.Log.LogWarning("Discarding too big serialized data for the format: " + FourCC(ItGlobal->first));
					ItGlobal = Globals.erase(ItGlobal);
				}
				else ++ItGlobal;
			}
		}

		// Process render targets

		struct CRenderTarget
		{
			vector4 ClearValue;
		};

		std::map<CStrID, CRenderTarget> RenderTargets;
		auto ItRenderTargets = Desc.find(CStrID("RenderTargets"));
		if (ItRenderTargets != Desc.cend())
		{
			if (!ItRenderTargets->second.IsA<Data::CParams>())
			{
				Task.Log.LogError("'RenderTargets' must be a section");
				return false;
			}

			const auto& RenderTargetsDesc = ItRenderTargets->second.GetValue<Data::CParams>();
			for (const auto& RTPair : RenderTargetsDesc)
			{
				if (!RTPair.second.IsA<Data::CParams>())
				{
					Task.Log.LogError("Render target '" + RTPair.first.ToString() + "' must be a section");
					return false;
				}

				const auto& RTDesc = RTPair.second.GetValue<Data::CParams>();
				CRenderTarget RT;
				RT.ClearValue = GetParam(RTDesc, "ClearValue", vector4{ 0.f, 0.f, 0.f, 1.f });
				RenderTargets.emplace(RTPair.first, std::move(RT));
			}
		}

		// Process depth-stencil buffers

		struct CDepthStencilBuffer
		{
			float DepthClearValue;
			uint8_t StencilClearValue;
		};

		std::map<CStrID, CDepthStencilBuffer> DepthStencilBuffers;
		auto ItDepthStencilBuffers = Desc.find(CStrID("DepthStencilBuffers"));
		if (ItDepthStencilBuffers != Desc.cend())
		{
			if (!ItDepthStencilBuffers->second.IsA<Data::CParams>())
			{
				Task.Log.LogError("'DepthStencilBuffers' must be a section");
				return false;
			}

			const auto& DepthStencilBuffersDesc = ItDepthStencilBuffers->second.GetValue<Data::CParams>();
			for (const auto& DSPair : DepthStencilBuffersDesc)
			{
				if (!DSPair.second.IsA<Data::CParams>())
				{
					Task.Log.LogError("Depth-stencil buffer '" + DSPair.first.ToString() + "' must be a section");
					return false;
				}

				const auto& DSDesc = DSPair.second.GetValue<Data::CParams>();
				CDepthStencilBuffer DS;
				DS.DepthClearValue = GetParam(DSDesc, "DepthClearValue", 1.f);
				DS.StencilClearValue = GetParam(DSDesc, "StencilClearValue", 0);
				DepthStencilBuffers.emplace(DSPair.first, std::move(DS));
			}
		}

		// Process phases

		auto ItPhases = Desc.find(CStrID("Phases"));
		if (ItPhases == Desc.cend() || !ItPhases->second.IsA<Data::CParams>())
		{
			Task.Log.LogError("'Phases' must be a section");
			return false;
		}

		const auto& PhaseDescs = ItPhases->second.GetValue<Data::CParams>();
		for (const auto& PhasePair : PhaseDescs)
		{
			if (!PhasePair.second.IsA<Data::CParams>())
			{
				Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' must be a section");
				return false;
			}

			const auto& PhaseDesc = PhasePair.second.GetValue<Data::CParams>();

			const auto Type = GetParam<std::string>(PhaseDesc, "Type", {});
			if (Type.empty())
			{
				Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' type not specified, Type = \"<string>\" expected");
				return false;
			}

			auto ItRT = PhaseDesc.find(CStrID("RenderTarget"));
			if (ItRT != PhaseDesc.cend())
			{
				if (ItRT->second.IsA<CStrID>())
				{
					const CStrID RefID = ItRT->second.GetValue<CStrID>();
					if (RenderTargets.find(RefID) == RenderTargets.cend())
					{
						Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown render target '" + RefID.ToString() + '\'');
						return false;
					}
				}
				else if (ItRT->second.IsA<Data::CDataArray>())
				{
					for (const auto& Ref : ItRT->second.GetValue<Data::CDataArray>())
					{
						if (!Ref.IsA<CStrID>())
						{
							Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' render target array must contain only string ID elements");
							return false;
						}

						const CStrID RefID = Ref.GetValue<CStrID>();
						if (RenderTargets.find(RefID) == RenderTargets.cend())
						{
							Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown render target '" + RefID.ToString() + '\'');
							return false;
						}
					}
				}
				else if (!ItRT->second.IsVoid())
				{
					Task.Log.LogWarning("Phase '" + PhasePair.first.ToString() + "' declares 'RenderTarget' which is not a string ID or an array of them");
				}
			}

			auto ItDS = PhaseDesc.find(CStrID("DepthStencilBuffer"));
			if (ItDS != PhaseDesc.cend())
			{
				if (ItDS->second.IsA<CStrID>())
				{
					const CStrID RefID = ItDS->second.GetValue<CStrID>();
					if (DepthStencilBuffers.find(RefID) == DepthStencilBuffers.cend())
					{
						Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown depth-stencil buffer '" + RefID.ToString() + '\'');
						return false;
					}
				}
				else if (!ItDS->second.IsVoid())
				{
					Task.Log.LogWarning("Phase '" + PhasePair.first.ToString() + "' declares 'DepthStencilBuffer' which is not a string ID");
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

		const auto ShaderFormatCount = static_cast<uint32_t>(Globals.size());

		WriteStream<uint32_t>(File, 'RPTH');     // Format magic value
		WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0

		// Write render targets
		WriteStream<uint32_t>(File, static_cast<uint32_t>(RenderTargets.size()));
		for (const auto& Pair : RenderTargets)
		{
			WriteStream(File, Pair.first.ToString());
			WriteStream(File, Pair.second.ClearValue);
		}

		// Write depth-stencil buffers
		WriteStream<uint32_t>(File, static_cast<uint32_t>(DepthStencilBuffers.size()));
		for (const auto& Pair : DepthStencilBuffers)
		{
			WriteStream(File, Pair.first.ToString());
			WriteStream(File, Pair.second.DepthClearValue);
			WriteStream(File, Pair.second.StencilClearValue);
		}

		// Write phases
		WriteStream(File, PhaseDescs);

		// Write format map (FourCC to offset from the body start)
		WriteStream<uint32_t>(File, ShaderFormatCount);
		uint32_t TotalOffset = static_cast<uint32_t>(File.tellp()) + ShaderFormatCount * 2 * sizeof(uint32_t);
		for (const auto& Pair : Globals)
		{
			WriteStream<uint32_t>(File, Pair.first);
			WriteStream<uint32_t>(File, TotalOffset);
			TotalOffset += static_cast<uint32_t>(Pair.second.Result.size()); // Overflow already checked
		}

		// Write serialized global parameter tables
		for (const auto& Pair : Globals)
			File.write(Pair.second.Result.c_str(), Pair.second.Result.size());

		return true;
	}
};

int main(int argc, const char** argv)
{
	CRenderPathTool Tool("cf-rpath", "DeusExMachina render path compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
