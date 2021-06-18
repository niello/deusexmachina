#include <CFRenderPathFwd.h>
#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>
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
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: CStrID
		return false;
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const std::string Output = ParamsUtils::GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".rp");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read render path hrd

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return ETaskResult::Failure;
		}

		// Build and verify global parameter table

		std::map<uint32_t, CGlobalTable> Globals;
		const Data::CData* pEffectPathes;
		if (ParamsUtils::TryGetParam(pEffectPathes, Desc, "Effects"))
		{
			if (!pEffectPathes->IsA<Data::CDataArray>())
			{
				Task.Log.LogError("'Phases' must be an array of pathes to .eff files");
				return ETaskResult::Failure;
			}

			// Collect global metadata sources

			const auto& EffectPathes = pEffectPathes->GetValue<Data::CDataArray>();
			for (const auto& EffectPathData : EffectPathes)
			{
				if (!EffectPathData.IsA<std::string>())
				{
					Task.Log.LogError("Wrong data in 'Effects' array, all elements must be strings");
					return ETaskResult::Failure;
				}

				auto Path = ResolvePathAliases(EffectPathData.GetValue<std::string>(), _PathAliases);
				Task.Log.LogDebug("Opening effect " + Path.generic_string());

				auto FilePtr = std::make_shared<std::ifstream>(Path, std::ios_base::binary);
				auto& File = *FilePtr;
				if (!File)
				{
					Task.Log.LogError("Can't open effect " + Path.generic_string());
					return ETaskResult::Failure;
				}

				if (ReadStream<uint32_t>(File) != 'SHFX')
				{
					Task.Log.LogError("Wrong effect file format in " + Path.generic_string());
					return ETaskResult::Failure;
				}

				if (ReadStream<uint32_t>(File) > 0x00010000)
				{
					Task.Log.LogError("Unsupported effect version in " + Path.generic_string());
					return ETaskResult::Failure;
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
						if (!BuildGlobalsTableForDX9C(FormatToGlobals.second, &Task.Log)) return ETaskResult::Failure;
						break;
					}
					case 'DXBC':
					{
						if (!BuildGlobalsTableForDXBC(FormatToGlobals.second, &Task.Log)) return ETaskResult::Failure;
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
			float4 ClearValue;
		};

		std::map<CStrID, CRenderTarget> RenderTargets;
		const Data::CData* pRenderTargets;
		if (ParamsUtils::TryGetParam(pRenderTargets, Desc, "RenderTargets"))
		{
			if (!pRenderTargets->IsA<Data::CParams>())
			{
				Task.Log.LogError("'RenderTargets' must be a section");
				return ETaskResult::Failure;
			}

			const auto& RenderTargetsDesc = pRenderTargets->GetValue<Data::CParams>();
			for (const auto& RTPair : RenderTargetsDesc)
			{
				if (!RTPair.second.IsA<Data::CParams>())
				{
					Task.Log.LogError("Render target '" + RTPair.first.ToString() + "' must be a section");
					return ETaskResult::Failure;
				}

				const auto& RTDesc = RTPair.second.GetValue<Data::CParams>();
				CRenderTarget RT;
				RT.ClearValue = ParamsUtils::GetParam(RTDesc, "ClearValue", float4{ 0.f, 0.f, 0.f, 1.f });
				RenderTargets.emplace(RTPair.first, std::move(RT));
			}
		}

		// Process depth-stencil buffers

		constexpr uint8_t Flag_ClearDepth = (1 << 0);
		constexpr uint8_t Flag_ClearStencil = (1 << 1);
		struct CDepthStencilBuffer
		{
			float DepthClearValue = 1.f;
			uint8_t StencilClearValue = 0;
			uint8_t ClearFlags = 0;
		};

		std::map<CStrID, CDepthStencilBuffer> DepthStencilBuffers;
		const Data::CData* pDepthStencilBuffers;
		if (ParamsUtils::TryGetParam(pDepthStencilBuffers, Desc, "DepthStencilBuffers"))
		{
			if (!pDepthStencilBuffers->IsA<Data::CParams>())
			{
				Task.Log.LogError("'DepthStencilBuffers' must be a section");
				return ETaskResult::Failure;
			}

			const auto& DepthStencilBuffersDesc = pDepthStencilBuffers->GetValue<Data::CParams>();
			for (const auto& DSPair : DepthStencilBuffersDesc)
			{
				if (!DSPair.second.IsA<Data::CParams>())
				{
					Task.Log.LogError("Depth-stencil buffer '" + DSPair.first.ToString() + "' must be a section");
					return ETaskResult::Failure;
				}

				const auto& DSDesc = DSPair.second.GetValue<Data::CParams>();
				CDepthStencilBuffer DS;

				if (ParamsUtils::TryGetParam(DS.DepthClearValue, DSDesc, "DepthClearValue"))
					DS.ClearFlags |= Flag_ClearDepth;

				int StencilClearValue;
				if (ParamsUtils::TryGetParam(StencilClearValue, DSDesc, "StencilClearValue"))
				{
					DS.StencilClearValue = static_cast<uint8_t>(StencilClearValue);
					DS.ClearFlags |= Flag_ClearStencil;
				}

				DepthStencilBuffers.emplace(DSPair.first, std::move(DS));
			}
		}

		// Process global params binding

		const Data::CParams* pGlobalParams;
		if (!ParamsUtils::TryGetParam(pGlobalParams, Desc, "Globals"))
		{
			Task.Log.LogError("'Globals' must be a section");
			return ETaskResult::Failure;
		}

		std::map<CStrID, std::string> GlobalParams;
		for (const auto& GlobalPair : *pGlobalParams)
		{
			std::string Value;
			if (GlobalPair.second.IsA<std::string>())
				Value = GlobalPair.second.GetValue<std::string>();
			else if (GlobalPair.second.IsA<CStrID>())
				Value = GlobalPair.second.GetValue<CStrID>().ToString();
			else
			{
				// TODO: can add hardcoded values of non-string types?
				Task.Log.LogError("Global param '" + GlobalPair.first.ToString() + "' must be a string or string ID");
				return ETaskResult::Failure;
			}

			if (Value.empty())
			{
				Task.Log.LogWarning("Empty global param '" + GlobalPair.first.ToString() + "' is skipped");
				continue;
			}

			//for (const auto& [ShaderFormatCode, Table] : Globals)
			//	if (!Table.Contains(Value)) Warning()!

			GlobalParams.emplace(GlobalPair.first, std::move(Value));
		}

		// Process phases

		const Data::CParams* pPhases;
		if (!ParamsUtils::TryGetParam(pPhases, Desc, "Phases"))
		{
			Task.Log.LogError("'Phases' must be a section");
			return ETaskResult::Failure;
		}

		const auto& PhaseDescs = *pPhases;
		for (const auto& PhasePair : PhaseDescs)
		{
			if (!PhasePair.second.IsA<Data::CParams>())
			{
				Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' must be a section");
				return ETaskResult::Failure;
			}

			const auto& PhaseDesc = PhasePair.second.GetValue<Data::CParams>();

			const auto Type = ParamsUtils::GetParam<std::string>(PhaseDesc, "Type", {});
			if (Type.empty())
			{
				Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' type not specified, Type = \"<string>\" expected");
				return ETaskResult::Failure;
			}

			const Data::CData* pRT;
			if (ParamsUtils::TryGetParam(pRT, PhaseDesc, "RenderTarget"))
			{
				if (pRT->IsA<CStrID>())
				{
					const CStrID RefID = pRT->GetValue<CStrID>();
					if (RenderTargets.find(RefID) == RenderTargets.cend())
					{
						Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown render target '" + RefID.ToString() + '\'');
						return ETaskResult::Failure;
					}
				}
				else if (pRT->IsA<Data::CDataArray>())
				{
					for (const auto& Ref : pRT->GetValue<Data::CDataArray>())
					{
						if (!Ref.IsA<CStrID>())
						{
							Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' render target array must contain only string ID elements");
							return ETaskResult::Failure;
						}

						const CStrID RefID = Ref.GetValue<CStrID>();
						if (RenderTargets.find(RefID) == RenderTargets.cend())
						{
							Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown render target '" + RefID.ToString() + '\'');
							return ETaskResult::Failure;
						}
					}
				}
				else if (!pRT->IsVoid())
				{
					Task.Log.LogWarning("Phase '" + PhasePair.first.ToString() + "' declares 'RenderTarget' which is not a string ID or an array of them");
				}
			}

			const Data::CData* pDS;
			if (ParamsUtils::TryGetParam(pDS, PhaseDesc, "DepthStencilBuffer"))
			{
				if (pDS->IsA<CStrID>())
				{
					const CStrID RefID = pDS->GetValue<CStrID>();
					if (DepthStencilBuffers.find(RefID) == DepthStencilBuffers.cend())
					{
						Task.Log.LogError("Phase '" + PhasePair.first.ToString() + "' references unknown depth-stencil buffer '" + RefID.ToString() + '\'');
						return ETaskResult::Failure;
					}
				}
				else if (!pDS->IsVoid())
				{
					Task.Log.LogWarning("Phase '" + PhasePair.first.ToString() + "' declares 'DepthStencilBuffer' which is not a string ID");
				}
			}
		}

		// Write resulting file

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
		if (!File)
		{
			Task.Log.LogError("Error opening an output file");
			return ETaskResult::Failure;
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
			WriteStream(File, Pair.second.ClearFlags);
		}

		// Write global params
		WriteStream<uint32_t>(File, static_cast<uint32_t>(GlobalParams.size()));
		for (const auto& [Name, Value] : GlobalParams)
		{
			WriteStream(File, Name.ToString());
			WriteStream(File, Value);
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
		// NB: metadata size is not written because skipping RP globals is not needed anywhere
		for (const auto& Pair : Globals)
			File.write(Pair.second.Result.c_str(), Pair.second.Result.size());

		return ETaskResult::Success;
	}
};

int main(int argc, const char** argv)
{
	CRenderPathTool Tool("cf-rpath", "DeusExMachina render path compiler", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
