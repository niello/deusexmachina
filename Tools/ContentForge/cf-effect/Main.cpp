#include <ContentForgeTool.h>
#include <RenderState.h>
#include <Utils.h>
#include <HRDParser.h>
//#include <CLI11.hpp>
//#include <mutex>
#include <set>
#include <thread>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/effects --path Data ../../../content

struct CTechnique
{
	std::vector<const CRenderState*> Passes;

	uint32_t ShaderFormatFourCC = 0;
	uint32_t MinFeatureLevel = 0;

	bool operator <(const CTechnique& Other)
	{
		// Sort by features descending, so we start with the
		// most interesting tech and fall back to simpler ones
		return MinFeatureLevel > Other.MinFeatureLevel;
	}
};

#pragma pack(push, 1)
struct CShaderHeader
{
	uint32_t Format;
	uint32_t MinFeatureLevel;
	uint8_t Type;
	uint32_t BinaryOffset;
};
#pragma pack(pop)

struct CShaderData
{
	CShaderHeader Header;
	std::unique_ptr<char[]> MetaBytes;
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
		std::map<CStrID, CShaderData> ShaderCache;

		auto& RenderStateDescs = ItRenderStates->second.GetValue<Data::CParams>();
		const auto& InputSetDescs = ItTechs->second.GetValue<Data::CParams>();
		for (const auto& InputSetDesc : InputSetDescs)
		{
			if (!InputSetDesc.second.IsA<Data::CParams>() || InputSetDesc.second.GetValue<Data::CParams>().empty())
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Input set '" << InputSetDesc.first.CStr() << "' must be a section containing at least one technique" << LineEnd;
				return false;
			}

			std::vector<CTechnique> InputSetTechs;

			const auto& TechDescs = InputSetDesc.second.GetValue<Data::CParams>();
			for (const auto& TechDesc : TechDescs)
			{
				if (!TechDesc.second.IsA<Data::CDataArray>() || TechDesc.second.GetValue<Data::CDataArray>().empty())
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << "Tech '" << InputSetDesc.first.CStr() << '.' << TechDesc.first.CStr() << "' must be an array containing at least one render state ID" << LineEnd;
					return false;
				}

				CTechnique Tech;

				const auto& PassDescs = TechDesc.second.GetValue<Data::CDataArray>();
				for (const auto& PassDesc : PassDescs)
				{
					const CStrID RenderStateID = PassDesc.GetValue<CStrID>();

					if (_LogVerbosity >= EVerbosity::Debug)
						std::cout << "Tech '" << InputSetDesc.first.CStr() << '.' << TechDesc.first.CStr() << "', pass '" << RenderStateID.CStr() << '\'' << LineEnd;

					auto ItRS = RSCache.find(RenderStateID);
					if (ItRS == RSCache.cend())
					{
						LoadRenderState(RSCache, ShaderCache, RenderStateID, RenderStateDescs);
						ItRS = RSCache.find(RenderStateID);
					}

					const auto& RS = ItRS->second;
					if (!RS.IsValid)
					{
						// Can discard only this tech, but for now issue an error and stop
						if (_LogVerbosity >= EVerbosity::Errors)
							std::cout << "Tech '" << InputSetDesc.first.CStr() << '.' << TechDesc.first.CStr() << "' uses invalid render state in a pass '" << RenderStateID.CStr() << '\'' << LineEnd;
						return false;
					}

					// TODO: allow DXBC+DXIL for D3D12? Will write DXIL as a state format? What if API supports DXIL only?
					if (Tech.ShaderFormatFourCC && Tech.ShaderFormatFourCC != RS.ShaderFormatFourCC)
					{
						if (_LogVerbosity >= EVerbosity::Errors)
							std::cout << "Tech '" << InputSetDesc.first.CStr() << '.' << TechDesc.first.CStr() << "' has unsupported mix of shader formats." << LineEnd;
						return false;
					}

					Tech.ShaderFormatFourCC = RS.ShaderFormatFourCC;

					if (Tech.MinFeatureLevel < RS.MinFeatureLevel)
						Tech.MinFeatureLevel = RS.MinFeatureLevel;

					// TODO: light count

					Tech.Passes.push_back(&RS);
				}

				InputSetTechs.push_back(std::move(Tech));
			}

			// See tech's operator <
			std::sort(InputSetTechs.begin(), InputSetTechs.end());
		}

		// FIXME: must be thread-safe, also can move to the common code
		//if (_LogVerbosity >= EVerbosity::Debug)
		//	std::cout << "Status: " << (Ok ? "OK" : "FAIL") << LineEnd << LineEnd;

		return true;
	}

	bool LoadRenderState(std::map<CStrID, CRenderState>& RSCache, std::map<CStrID, CShaderData>& ShaderCache,
		CStrID ID, Data::CParams& RenderStateDescs)
	{
		// Insert invalid render state initially not to do it in every 'return false' below.
		// We want to have a record for each parsed RS, not only for valid ones.
		CRenderState& RS = RSCache.emplace(ID, CRenderState{}).first->second;

		const auto LineEnd = std::cout.widen('\n');

		// Get a HRD section for this render state

		auto It = RenderStateDescs.find(ID);
		if (It == RenderStateDescs.cend() || !It->second.IsA<Data::CParams>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << "Render state '" << ID.CStr() << "' not found or is not a section" << LineEnd;
			return false;
		}

		// Intentionally non-const, see blend settings reading
		auto& Desc = It->second.GetValue<Data::CParams>();

		// Load base state if specified

		const CStrID BaseID = GetParam(Desc, "Base", CStrID::Empty);
		if (BaseID)
		{
			auto ItRS = RSCache.find(BaseID);
			if (ItRS == RSCache.cend())
			{
				LoadRenderState(RSCache, ShaderCache, BaseID, RenderStateDescs);
				ItRS = RSCache.find(BaseID);
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

		TryGetParam(RS.VertexShader, Desc, "VS");
		TryGetParam(RS.HullShader, Desc, "HS");
		TryGetParam(RS.DomainShader, Desc, "DS");
		TryGetParam(RS.GeometryShader, Desc, "GS");
		TryGetParam(RS.PixelShader, Desc, "PS");

		std::string StrValue;
		int IntValue;
		bool FlagValue;
		vector4 Vector4Value;

		if (TryGetParam(StrValue, Desc, "Cull"))
		{
			trim(StrValue, " \r\n\t");
			ToLower(StrValue);
			if (StrValue == "none")
			{
				RS.Flags &= ~CRenderState::Rasterizer_CullFront;
				RS.Flags &= ~CRenderState::Rasterizer_CullBack;
			}
			else if (StrValue == "front")
			{
				RS.Flags |= CRenderState::Rasterizer_CullFront;
				RS.Flags &= ~CRenderState::Rasterizer_CullBack;
			}
			else if (StrValue == "back")
			{
				RS.Flags &= ~CRenderState::Rasterizer_CullFront;
				RS.Flags |= CRenderState::Rasterizer_CullBack;
			}
			else
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Render state '" << ID.CStr() << "' has unknown Cull value: " << StrValue << LineEnd;
				return false;
			}
		}

		if (TryGetParam(FlagValue, Desc, "Wireframe"))
			RS.SetFlags(CRenderState::Rasterizer_Wireframe, FlagValue);
		if (TryGetParam(FlagValue, Desc, "FrontCCW"))
			RS.SetFlags(CRenderState::Rasterizer_FrontCCW, FlagValue);
		if (TryGetParam(FlagValue, Desc, "DepthClipEnable"))
			RS.SetFlags(CRenderState::Rasterizer_DepthClipEnable, FlagValue);
		if (TryGetParam(FlagValue, Desc, "ScissorEnable"))
			RS.SetFlags(CRenderState::Rasterizer_ScissorEnable, FlagValue);
		if (TryGetParam(FlagValue, Desc, "MSAAEnable"))
			RS.SetFlags(CRenderState::Rasterizer_MSAAEnable, FlagValue);
		if (TryGetParam(FlagValue, Desc, "MSAALinesEnable"))
			RS.SetFlags(CRenderState::Rasterizer_MSAALinesEnable, FlagValue);

		TryGetParam(RS.DepthBias, Desc, "DepthBias");
		TryGetParam(RS.DepthBiasClamp, Desc, "DepthBiasClamp");
		TryGetParam(RS.SlopeScaledDepthBias, Desc, "SlopeScaledDepthBias");

		if (TryGetParam(FlagValue, Desc, "DepthEnable"))
			RS.SetFlags(CRenderState::DS_DepthEnable, FlagValue);
		if (TryGetParam(FlagValue, Desc, "DepthWriteEnable"))
			RS.SetFlags(CRenderState::DS_DepthWriteEnable, FlagValue);
		if (TryGetParam(FlagValue, Desc, "StencilEnable"))
			RS.SetFlags(CRenderState::DS_StencilEnable, FlagValue);

		if (TryGetParam(StrValue, Desc, "DepthFunc"))
			RS.DepthFunc = StringToCmpFunc(StrValue);

		if (TryGetParam(IntValue, Desc, "StencilReadMask"))
			RS.StencilReadMask = IntValue;
		if (TryGetParam(IntValue, Desc, "StencilWriteMask"))
			RS.StencilWriteMask = IntValue;
		if (TryGetParam(StrValue, Desc, "StencilFrontFunc"))
			RS.StencilFrontFace.StencilFunc = StringToCmpFunc(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilFrontPassOp"))
			RS.StencilFrontFace.StencilPassOp = StringToStencilOp(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilFrontFailOp"))
			RS.StencilFrontFace.StencilFailOp = StringToStencilOp(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilFrontDepthFailOp"))
			RS.StencilFrontFace.StencilDepthFailOp = StringToStencilOp(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilBackFunc"))
			RS.StencilBackFace.StencilFunc = StringToCmpFunc(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilBackPassOp"))
			RS.StencilBackFace.StencilPassOp = StringToStencilOp(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilBackFailOp"))
			RS.StencilBackFace.StencilFailOp = StringToStencilOp(StrValue);
		if (TryGetParam(StrValue, Desc, "StencilBackDepthFailOp"))
			RS.StencilBackFace.StencilDepthFailOp = StringToStencilOp(StrValue);
		if (TryGetParam(IntValue, Desc, "StencilRef"))
			RS.StencilRef = IntValue;

		if (TryGetParam(FlagValue, Desc, "AlphaToCoverage"))
			RS.SetFlags(CRenderState::Blend_AlphaToCoverage, FlagValue);

		if (TryGetParam(Vector4Value, Desc, "BlendFactor"))
			memcpy(RS.BlendFactorRGBA, &Vector4Value, sizeof(vector4));
		if (TryGetParam(IntValue, Desc, "SampleMask"))
			RS.SampleMask = IntValue;

		auto ItBlend = Desc.find(CStrID("Blend"));
		if (ItBlend != Desc.cend())
		{
			if (ItBlend->second.IsA<Data::CParams>())
			{
				// Slightly simplify further processing
				Data::CDataArray HelperArray;
				HelperArray.push_back(std::move(ItBlend->second));
				ItBlend->second.SetValue<Data::CDataArray>(HelperArray);

				RS.Flags &= ~CRenderState::Blend_Independent;
			}
			else if (ItBlend->second.IsA<Data::CDataArray>())
			{
				RS.Flags |= CRenderState::Blend_Independent;
			}
			else
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Render state '" << ID.CStr() << "' has invalid 'Blend'. Must be a section or an array." << LineEnd;
				return false;
			}

			const auto& BlendDescs = ItBlend->second.GetValue<Data::CDataArray>();
			if (BlendDescs.size() > 8)
			{
				if (_LogVerbosity >= EVerbosity::Warnings)
					std::cout << "Render state '" << ID.CStr() << "' has 'Blend' array of size " << BlendDescs.size() << ", note that only 8 first elements will be processed" << LineEnd;
			}

			// Read blend descs one by one

			for (size_t i = 0; i < BlendDescs.size() && i < 8; ++i)
			{
				if (!BlendDescs[i].IsA<Data::CParams>())
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << "Render state '" << ID.CStr() << "' has invalid 'Blend'. All array elements must be sections." << LineEnd;
					return false;
				}

				const auto& BlendDesc = BlendDescs[i].GetValue<Data::CParams>();

				if (TryGetParam(FlagValue, BlendDesc, "Enable"))
					RS.SetFlags(CRenderState::Blend_RTBlendEnable << i, FlagValue);

				CRenderState::CRTBlend& Blend = RS.RTBlend[i];

				if (TryGetParam(StrValue, BlendDesc, "SrcBlendArg"))
					Blend.SrcBlendArg = StringToBlendArg(StrValue);
				if (TryGetParam(StrValue, BlendDesc, "SrcBlendArgAlpha"))
					Blend.SrcBlendArgAlpha = StringToBlendArg(StrValue);
				if (TryGetParam(StrValue, BlendDesc, "DestBlendArg"))
					Blend.DestBlendArg = StringToBlendArg(StrValue);
				if (TryGetParam(StrValue, BlendDesc, "DestBlendArgAlpha"))
					Blend.DestBlendArgAlpha = StringToBlendArg(StrValue);
				if (TryGetParam(StrValue, BlendDesc, "BlendOp"))
					Blend.BlendOp = StringToBlendOp(StrValue);
				if (TryGetParam(StrValue, BlendDesc, "BlendOpAlpha"))
					Blend.BlendOpAlpha = StringToBlendOp(StrValue);

				auto ItWriteMask = BlendDesc.find(CStrID("WriteMask"));
				if (ItWriteMask != BlendDesc.cend())
				{
					if (ItWriteMask->second.IsVoid())
						Blend.WriteMask = 0;
					else if (ItWriteMask->second.IsA<int>())
						Blend.WriteMask = ItWriteMask->second.GetValue<int>() & 0x0f;
					else if (ItWriteMask->second.IsA<std::string>())
					{
						StrValue = ItWriteMask->second.GetValue<std::string>();
						trim(StrValue, " \r\n\t");
						ToLower(StrValue);

						Blend.WriteMask = 0;
						if (StrValue.find_first_of('r') != std::string::npos)
							Blend.WriteMask = ColorMask_Red;
						if (StrValue.find_first_of('g') != std::string::npos)
							Blend.WriteMask = ColorMask_Green;
						if (StrValue.find_first_of('b') != std::string::npos)
							Blend.WriteMask = ColorMask_Blue;
						if (StrValue.find_first_of('a') != std::string::npos)
							Blend.WriteMask = ColorMask_Alpha;
					}
					else
					{
						if (_LogVerbosity >= EVerbosity::Errors)
							std::cout << "Render state '" << ID.CStr() << "' has invalid 'Blend'. 'WriteMask' must be an int or a string, empty or containing r, g, b & a in any combination." << LineEnd;
						return false;
					}
				}
			}
		}

		if (TryGetParam(FlagValue, Desc, "AlphaTestEnable"))
			RS.SetFlags(CRenderState::Misc_AlphaTestEnable, FlagValue);
		if (TryGetParam(IntValue, Desc, "AlphaTestRef"))
			RS.AlphaTestRef = IntValue;
		if (TryGetParam(StrValue, Desc, "AlphaTestFunc"))
			RS.AlphaTestFunc = StringToCmpFunc(StrValue);

		auto ItClipPlanes = Desc.find(CStrID("ClipPlanes"));
		if (ItClipPlanes != Desc.cend())
		{
			if (ItClipPlanes->second.IsA<bool>() && ItClipPlanes->second == false)
			{
				// All clip planes disabled
			}
			else if (ItClipPlanes->second.IsA<int>())
			{
				const int CP = ItClipPlanes->second;
				for (int i = 0; i < 5; ++i)
					RS.SetFlags(CRenderState::Misc_ClipPlaneEnable << i, !!(CP & (1 << i)));
			}
			else if (ItClipPlanes->second.IsA<Data::CDataArray>())
			{			
				for (size_t i = 0; i < 5; ++i)
					RS.Flags &= ~(CRenderState::Misc_ClipPlaneEnable << i);
			
				const Data::CDataArray& CP = ItClipPlanes->second.GetValue<Data::CDataArray>();
				for (const auto& Val : CP)
				{
					if (!Val.IsA<int>()) continue;
					int IntVal = Val;
					if (IntVal < 0 || IntVal > 5) continue;
					RS.Flags |= (CRenderState::Misc_ClipPlaneEnable << IntVal);
				}
			}
			else
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Render state '" << ID.CStr() << "' has invalid 'ClipPlanes'. Must be 'false', int bitmask or an array of indices." << LineEnd;
				return false;
			}
		}

		// Process shaders

		std::set<CStrID> Shaders;
		if (RS.VertexShader) Shaders.insert(RS.VertexShader);
		if (RS.HullShader) Shaders.insert(RS.HullShader);
		if (RS.DomainShader) Shaders.insert(RS.DomainShader);
		if (RS.GeometryShader) Shaders.insert(RS.GeometryShader);
		if (RS.PixelShader) Shaders.insert(RS.PixelShader);

		for (CStrID ShaderID : Shaders)
		{
			auto ShaderDataIt = ShaderCache.find(ShaderID);
			if (ShaderDataIt == ShaderCache.cend())
			{
				auto Path = ResolvePathAliases(ShaderID.CStr());

				if (_LogVerbosity >= EVerbosity::Debug)
					std::cout << "Opening shader " << Path.generic_string() << LineEnd;

				std::ifstream File(Path, std::ios_base::binary);
				if (!File)
				{
					if (_LogVerbosity >= EVerbosity::Errors)
						std::cout << "Can't open shader " << Path.generic_string() << LineEnd;
					return false;
				}

				CShaderData ShaderData;
				ReadStream(File, ShaderData.Header);

				// TODO: get light count
				// collect max light count of all shaders
				// shader switching is costly, so we don't build per-light-count variations, but instead
				// create the shader with max light count only
				//???how to calculate max light count and whether the shader uses lights at all?
				//???parameter in a shader metafile? will add DEM_MAX_LIGHT_COUNT definition to the compiler + metadata
				//???or find a way to determine light count from shader metadata?

				// Cache metadata bytes
				const size_t MetaSize = ShaderData.Header.BinaryOffset - sizeof(CShaderHeader);
				if (MetaSize)
				{
					ShaderData.MetaBytes.reset(new char[MetaSize]);
					if (!File.read(ShaderData.MetaBytes.get(), MetaSize))
					{
						if (_LogVerbosity >= EVerbosity::Errors)
							std::cout << "Render state '" << ID.CStr() << "' metadata bytes reading error." << LineEnd;
						return false;
					}
				}

				ShaderDataIt = ShaderCache.emplace(ShaderID, std::move(ShaderData)).first;
			}

			const CShaderData& ShaderData = ShaderDataIt->second;

			// TODO: allow DXBC+DXIL for D3D12? Will write DXIL as a state format? What if API supports DXIL only?
			if (RS.ShaderFormatFourCC && RS.ShaderFormatFourCC != ShaderData.Header.Format)
			{
				if (_LogVerbosity >= EVerbosity::Errors)
					std::cout << "Render state '" << ID.CStr() << "' has unsupported mix of shader formats." << LineEnd;
				return false;
			}

			RS.ShaderFormatFourCC = ShaderData.Header.Format;

			if (RS.MinFeatureLevel < ShaderData.Header.MinFeatureLevel)
				RS.MinFeatureLevel = ShaderData.Header.MinFeatureLevel;
		}

		RS.IsValid = true;
		return true;
	}
};

int main(int argc, const char** argv)
{
	CEffectTool Tool("cf-effect", "DeusExMachina rendering effect compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
