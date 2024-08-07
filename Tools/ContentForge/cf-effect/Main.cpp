#include <ContentForgeTool.h>
#include <CFEffectFwd.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <Render/ShaderMetaCommon.h>
#include <thread>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/effects --path Data ../../../content

bool WriteParameterTablesForDX9C(std::ostream& Stream, std::vector<CTechnique>& Techs, CMaterialParams& MaterialParams, const CContext& Ctx);
bool WriteParameterTablesForDXBC(std::ostream& Stream, std::vector<CTechnique>& Techs, CMaterialParams& MaterialParams, const CContext& Ctx);

class CEffectTool : public CContentForgeTool
{
private:

public:

	CEffectTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const auto DestPath = GetOutputPath(Task.Params) / (Task.TaskID.ToString() + ".eff");

		// Read effect hrd

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return ETaskResult::Failure;
		}

		// Get and validate material type

		const CStrID MaterialType = ParamsUtils::GetParam<CStrID>(Desc, "Type", CStrID::Empty);
		if (!MaterialType)
		{
			Task.Log.LogError("Material type not specified, Type = '<string ID>' expected");
			return ETaskResult::Failure;
		}

		// Get and validate global and material params

		CContext Ctx;
		Ctx.Log = &Task.Log;

		const Data::CData* pGlobalParams;
		ParamsUtils::TryGetParam(pGlobalParams, Desc, "GlobalParams");
		const bool HasGlobalParams = pGlobalParams && !pGlobalParams->IsVoid();
		if (HasGlobalParams)
		{
			if (!pGlobalParams->IsA<Data::CDataArray>())
			{
				Task.Log.LogError("'GlobalParams' must be an array of CStringID elements (single-quoted strings)");
				return ETaskResult::Failure;
			}

			const auto& GlobalParamsDesc = pGlobalParams->GetValue<Data::CDataArray>();
			for (const auto& Data : GlobalParamsDesc)
			{
				if (!Data.IsA<CStrID>())
				{
					Task.Log.LogError("'GlobalParams' must be an array of CStringID elements (single-quoted strings)");
					return ETaskResult::Failure;
				}

				Ctx.GlobalParams.insert(Data.GetValue<CStrID>());
			}
		}

		const Data::CData* pMaterialParams;
		ParamsUtils::TryGetParam(pMaterialParams, Desc, "MaterialParams");
		const bool HasMaterialParams = pMaterialParams && !pMaterialParams->IsVoid();
		if (HasMaterialParams)
		{
			if (!pMaterialParams->IsA<Data::CParams>())
			{
				Task.Log.LogError("'MaterialParams' must be a section");
				return ETaskResult::Failure;
			}

			Ctx.MaterialParams = std::move(pMaterialParams->GetValue<Data::CParams>());
		}

		if (HasGlobalParams && HasMaterialParams)
		{
			// Check intersections between global and material params
			bool HasErrors = false;
			for (const auto& ParamID : Ctx.GlobalParams)
			{
				if (ParamsUtils::HasParam(Ctx.MaterialParams, ParamID))
				{
					Task.Log.LogError('\'' + ParamID.ToString() + "' appears in both global and material params");
					HasErrors = true;
				}
			}
		}

		// Parse, validate and sort techniques

		Data::CParams* pRenderStates;
		if (!ParamsUtils::TryGetParam(pRenderStates, Desc, "RenderStates"))
		{
			Task.Log.LogError("'RenderStates' must be a section");
			return ETaskResult::Failure;
		}

		const Data::CParams* pTechs;
		if (!ParamsUtils::TryGetParam(pTechs, Desc, "Techniques") || pTechs->empty())
		{
			Task.Log.LogError("'Techniques' must be a section and contain at least one input set");
			return ETaskResult::Failure;
		}

		auto& RenderStateDescs = *pRenderStates;
		const auto& InputSetDescs = *pTechs;
		for (const auto& InputSetDesc : InputSetDescs)
		{
			CStrID InputSet = InputSetDesc.first;

			if (!InputSetDesc.second.IsA<Data::CParams>() || InputSetDesc.second.GetValue<Data::CParams>().empty())
			{
				Task.Log.LogError("Input set '" + InputSet.ToString() + "' must be a section containing at least one technique");
				return ETaskResult::Failure;
			}

			const auto& TechDescs = InputSetDesc.second.GetValue<Data::CParams>();
			for (const auto& TechDesc : TechDescs)
			{
				if (!TechDesc.second.IsA<Data::CDataArray>() || TechDesc.second.GetValue<Data::CDataArray>().empty())
				{
					Task.Log.LogError("Tech '" + InputSet.ToString() + '.' + TechDesc.first.ToString() + "' must be an array containing at least one render state ID");
					return ETaskResult::Failure;
				}

				CTechnique Tech;

				const auto& PassDescs = TechDesc.second.GetValue<Data::CDataArray>();
				for (const auto& PassDesc : PassDescs)
				{
					const CStrID RenderStateID = PassDesc.GetValue<CStrID>();

					Task.Log.LogDebug("Tech '" + InputSet.ToString() + '.' + TechDesc.first.ToString() + "', pass '" + RenderStateID.CStr() + '\'');

					auto ItRS = Ctx.RSCache.find(RenderStateID);
					if (ItRS == Ctx.RSCache.cend())
					{
						LoadRenderState(Ctx, RenderStateID, RenderStateDescs);
						ItRS = Ctx.RSCache.find(RenderStateID);
					}

					const auto& RS = ItRS->second;
					if (!RS.IsValid)
					{
						// Can discard only this tech, but for now issue an error and stop
						Task.Log.LogError("Tech '" + InputSet.ToString() + '.' + TechDesc.first.ToString() + "' uses invalid render state in a pass '" + RenderStateID.CStr() + '\'');
						return ETaskResult::Failure;
					}

					// TODO: allow DXBC+DXIL for D3D12? Will write DXIL as a state format? What if API supports DXIL only?
					if (Tech.ShaderFormatFourCC && Tech.ShaderFormatFourCC != RS.ShaderFormatFourCC)
					{
						Task.Log.LogError("Tech '" + InputSet.ToString() + '.' + TechDesc.first.ToString() + "' has unsupported mix of shader formats.");
						return ETaskResult::Failure;
					}

					Tech.ShaderFormatFourCC = RS.ShaderFormatFourCC;

					if (Tech.MinFeatureLevel < RS.MinFeatureLevel)
						Tech.MinFeatureLevel = RS.MinFeatureLevel;

					// TODO: light count

					Tech.ID = TechDesc.first;
					Tech.InputSet = InputSet;

					Tech.Passes.push_back(RenderStateID);
				}

				Ctx.TechsByFormat[Tech.ShaderFormatFourCC].push_back(std::move(Tech));
			}
		}

		// Build resulting effect for each shader format separately

		CMaterialParams MaterialParams;
		std::map<uint32_t, std::string> SerializedEffect;
		for (auto& FormatTechs : Ctx.TechsByFormat)
		{
			const uint32_t ShaderFormat = FormatTechs.first;
			auto& Techs = FormatTechs.second;

			// Sort techs for easier processing, see tech's operator <
			std::sort(Techs.begin(), Techs.end());

			std::ostringstream Stream(std::ios_base::binary);

			// Serialize parameter tables

			switch (ShaderFormat)
			{
				case 'DX9C':
				{
					if (!WriteParameterTablesForDX9C(Stream, Techs, MaterialParams, Ctx)) return ETaskResult::Failure;
					break;
				}
				case 'DXBC':
				{
					if (!WriteParameterTablesForDXBC(Stream, Techs, MaterialParams, Ctx)) return ETaskResult::Failure;
					break;
				}
				default:
				{
					Task.Log.LogWarning("Skipping unsupported shader format: " + FourCC(ShaderFormat));
					continue;
				}
			}

			// Build a vector of used render states to reference them by index

			std::vector<CStrID> RenderStates;
			for (const auto& Tech : Techs)
				for (CStrID PassID : Tech.Passes)
					if (std::find(RenderStates.cbegin(), RenderStates.cend(), PassID) == RenderStates.cend())
						RenderStates.push_back(PassID);

			// Serialize techniques

			WriteStream<uint32_t>(Stream, static_cast<uint32_t>(Techs.size()));
			for (const auto& Tech : Techs)
			{
				auto InputSet = Tech.InputSet.ToString();
				auto PassCount = static_cast<uint32_t>(Tech.Passes.size());

				// Calculate offset of the next tech for fast skipping
				const uint32_t Offset =
					static_cast<uint32_t>(Stream.tellp()) + // Curr pos
					sizeof(uint32_t) + // MinFeatureLevel
					sizeof(uint32_t) + // Offset
					sizeof(uint16_t) + static_cast<uint32_t>(InputSet.size()) + // InputSet
					sizeof(uint32_t) + // Pass count
					PassCount * sizeof(uint32_t) + // Pass indices
					static_cast<uint32_t>(Tech.EffectMetaBinary.size()); // Tech params

				//WriteStream(Stream, Tech.ID.ToString()); // not used in an engine
				WriteStream(Stream, Tech.MinFeatureLevel);
				WriteStream(Stream, Offset);
				WriteStream(Stream, InputSet);

				WriteStream<uint32_t>(Stream, PassCount);
				for (CStrID PassID : Tech.Passes)
				{
					auto ItPass = std::find(RenderStates.cbegin(), RenderStates.cend(), PassID);
					WriteStream<uint32_t>(Stream, static_cast<uint32_t>(std::distance(RenderStates.cbegin(), ItPass)));
				}

				Stream.write(Tech.EffectMetaBinary.c_str(), Tech.EffectMetaBinary.size());

				assert(Offset == static_cast<uint32_t>(Stream.tellp()));
			}

			// Serialize render states

			WriteStream<uint32_t>(Stream, static_cast<uint32_t>(RenderStates.size()));
			for (CStrID RSID : RenderStates)
				SaveRenderState(Stream, Ctx.RSCache.at(RSID));

			// Check and store serialized data

			std::string Data = Stream.str();
			if (Data.empty())
			{
				Task.Log.LogWarning("No data serialized for the format: " + FourCC(ShaderFormat));
				continue;
			}
			else if (Data.size() > std::numeric_limits<uint32_t>().max())
			{
				// We don't support 64-bit offsets in effect files. If your data is so big,
				// most probably it is a serialization error.
				Task.Log.LogWarning("Discarding too big serialized data for the format: " + FourCC(ShaderFormat));
				continue;
			}

			SerializedEffect.emplace(ShaderFormat, std::move(Data));
		}

		// Write resulting file

		if (SerializedEffect.empty())
		{
			Task.Log.LogError("No data serialized for the effect, resource will not be created");
			return ETaskResult::Failure;
		}

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
		if (!File)
		{
			Task.Log.LogError("Error opening an output file");
			return ETaskResult::Failure;
		}

		const auto ShaderFormatCount = static_cast<uint32_t>(SerializedEffect.size());

		WriteStream<uint32_t>(File, 'SHFX');        // Format magic value
		WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
		WriteStream(File, MaterialType.ToString()); //???or write enum instead of string?

		// Write format map (FourCC to offset from the body start)
		WriteStream(File, ShaderFormatCount);
		uint32_t TotalOffset = static_cast<uint32_t>(File.tellp()) + ShaderFormatCount * 2 * sizeof(uint32_t) + sizeof(uint32_t);
		for (const auto& Pair : SerializedEffect)
		{
			WriteStream<uint32_t>(File, Pair.first);
			WriteStream<uint32_t>(File, TotalOffset);
			TotalOffset += static_cast<uint32_t>(Pair.second.size()); // Overflow already checked
		}

		// Write material defaults offset
		WriteStream<uint32_t>(File, TotalOffset);

		// Write serialized effect blocks
		for (const auto& Pair : SerializedEffect)
			File.write(Pair.second.c_str(), Pair.second.size());

		// Serialize material defaults
		if (!WriteMaterialParams(File, MaterialParams, Ctx.MaterialParams, Task.Log))
		{
			Task.Log.LogError("Error serializing material defaults");
			return ETaskResult::Failure;
		}

		return ETaskResult::Success;
	}

private:

	bool LoadRenderState(CContext& Ctx, CStrID ID, Data::CParams& RenderStateDescs)
	{
		// Insert invalid render state initially not to do it in every 'return false' below.
		// We want to have a record for each parsed RS, not only for valid ones.
		CRenderState& RS = Ctx.RSCache.emplace(ID, CRenderState{}).first->second;

		// Get a HRD section for this render state

		Data::CParams* pDesc;
		if (!ParamsUtils::TryGetParam(pDesc, RenderStateDescs, ID))
		{
			Ctx.Log->LogError("Render state '" + ID.ToString() + "' not found or is not a section");
			return false;
		}

		// Intentionally non-const, see blend settings reading
		auto& Desc = *pDesc;

		// Load base state if specified

		const CStrID BaseID = ParamsUtils::GetParam(Desc, "Base", CStrID::Empty);
		if (BaseID)
		{
			auto ItRS = Ctx.RSCache.find(BaseID);
			if (ItRS == Ctx.RSCache.cend())
			{
				LoadRenderState(Ctx, BaseID, RenderStateDescs);
				ItRS = Ctx.RSCache.find(BaseID);
			}

			const auto& BaseRS = ItRS->second;
			if (!BaseRS.IsValid)
			{
				Ctx.Log->LogError("Render state '" + ID.ToString() + "' has invalid base state '" + BaseID.CStr() + '\'');
				return false;
			}

			// Copy base state to the current, so all parameters in the current desc will override base values
			RS = BaseRS;
		}

		// Load explicit state

		ParamsUtils::TryGetParam(RS.VertexShader, Desc, "VS");
		ParamsUtils::TryGetParam(RS.HullShader, Desc, "HS");
		ParamsUtils::TryGetParam(RS.DomainShader, Desc, "DS");
		ParamsUtils::TryGetParam(RS.GeometryShader, Desc, "GS");
		ParamsUtils::TryGetParam(RS.PixelShader, Desc, "PS");

		std::string StrValue;
		int IntValue;
		bool FlagValue;
		float4 Vector4Value;

		if (ParamsUtils::TryGetParam(StrValue, Desc, "Cull"))
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
				Ctx.Log->LogError("Render state '" + ID.ToString() + "' has unknown Cull value: " + StrValue);
				return false;
			}
		}

		// Triangle winding is defaulted to CCW, because glTF 2.0, Maya and probably
		// other popular formats/software also default to it
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "FrontCCW"))
			RS.SetFlags(CRenderState::Rasterizer_FrontCCW, FlagValue);
		else
			RS.SetFlags(CRenderState::Rasterizer_FrontCCW, true);

		if (ParamsUtils::TryGetParam(FlagValue, Desc, "Wireframe"))
			RS.SetFlags(CRenderState::Rasterizer_Wireframe, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "DepthClipEnable"))
			RS.SetFlags(CRenderState::Rasterizer_DepthClipEnable, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "ScissorEnable"))
			RS.SetFlags(CRenderState::Rasterizer_ScissorEnable, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "MSAAEnable"))
			RS.SetFlags(CRenderState::Rasterizer_MSAAEnable, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "MSAALinesEnable"))
			RS.SetFlags(CRenderState::Rasterizer_MSAALinesEnable, FlagValue);

		ParamsUtils::TryGetParam(RS.DepthBias, Desc, "DepthBias");
		ParamsUtils::TryGetParam(RS.DepthBiasClamp, Desc, "DepthBiasClamp");
		ParamsUtils::TryGetParam(RS.SlopeScaledDepthBias, Desc, "SlopeScaledDepthBias");

		if (ParamsUtils::TryGetParam(FlagValue, Desc, "DepthEnable"))
			RS.SetFlags(CRenderState::DS_DepthEnable, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "DepthWriteEnable"))
			RS.SetFlags(CRenderState::DS_DepthWriteEnable, FlagValue);
		if (ParamsUtils::TryGetParam(FlagValue, Desc, "StencilEnable"))
			RS.SetFlags(CRenderState::DS_StencilEnable, FlagValue);

		if (ParamsUtils::TryGetParam(StrValue, Desc, "DepthFunc"))
			RS.DepthFunc = StringToCmpFunc(StrValue);

		if (ParamsUtils::TryGetParam(IntValue, Desc, "StencilReadMask"))
			RS.StencilReadMask = IntValue;
		if (ParamsUtils::TryGetParam(IntValue, Desc, "StencilWriteMask"))
			RS.StencilWriteMask = IntValue;
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilFrontFunc"))
			RS.StencilFrontFace.StencilFunc = StringToCmpFunc(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilFrontPassOp"))
			RS.StencilFrontFace.StencilPassOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilFrontFailOp"))
			RS.StencilFrontFace.StencilFailOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilFrontDepthFailOp"))
			RS.StencilFrontFace.StencilDepthFailOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilBackFunc"))
			RS.StencilBackFace.StencilFunc = StringToCmpFunc(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilBackPassOp"))
			RS.StencilBackFace.StencilPassOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilBackFailOp"))
			RS.StencilBackFace.StencilFailOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(StrValue, Desc, "StencilBackDepthFailOp"))
			RS.StencilBackFace.StencilDepthFailOp = StringToStencilOp(StrValue);
		if (ParamsUtils::TryGetParam(IntValue, Desc, "StencilRef"))
			RS.StencilRef = IntValue;

		if (ParamsUtils::TryGetParam(FlagValue, Desc, "AlphaToCoverage"))
			RS.SetFlags(CRenderState::Blend_AlphaToCoverage, FlagValue);

		if (ParamsUtils::TryGetParam(Vector4Value, Desc, "BlendFactor"))
			memcpy(RS.BlendFactorRGBA, &Vector4Value, sizeof(float4));
		if (ParamsUtils::TryGetParam(IntValue, Desc, "SampleMask"))
			RS.SampleMask = IntValue;

		Data::CData* pBlend;
		if (ParamsUtils::TryGetParam(pBlend, Desc, "Blend"))
		{
			if (pBlend->IsA<Data::CParams>())
			{
				// Slightly simplify further processing
				Data::CDataArray HelperArray;
				HelperArray.push_back(std::move(*pBlend));
				pBlend->SetValue<Data::CDataArray>(HelperArray);

				RS.Flags &= ~CRenderState::Blend_Independent;
			}
			else if (pBlend->IsA<Data::CDataArray>())
			{
				RS.Flags |= CRenderState::Blend_Independent;
			}
			else
			{
				Ctx.Log->LogError("Render state '" + ID.ToString() + "' has invalid 'Blend'. Must be a section or an array.");
				return false;
			}

			const auto& BlendDescs = pBlend->GetValue<Data::CDataArray>();
			if (BlendDescs.size() > 8)
				Ctx.Log->LogWarning("Render state '" + ID.ToString() + "' has 'Blend' array of size " + std::to_string(BlendDescs.size()) + ", note that only 8 first elements will be processed");

			// Read blend descs one by one

			for (size_t i = 0; i < BlendDescs.size() && i < 8; ++i)
			{
				if (!BlendDescs[i].IsA<Data::CParams>())
				{
					Ctx.Log->LogError("Render state '" + ID.ToString() + "' has invalid 'Blend'. All array elements must be sections.");
					return false;
				}

				const auto& BlendDesc = BlendDescs[i].GetValue<Data::CParams>();

				if (ParamsUtils::TryGetParam(FlagValue, BlendDesc, "Enable"))
					RS.SetFlags(CRenderState::Blend_RTBlendEnable << i, FlagValue);

				CRenderState::CRTBlend& Blend = RS.RTBlend[i];

				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "SrcBlendArg"))
					Blend.SrcBlendArg = StringToBlendArg(StrValue);
				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "SrcBlendArgAlpha"))
					Blend.SrcBlendArgAlpha = StringToBlendArg(StrValue);
				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "DestBlendArg"))
					Blend.DestBlendArg = StringToBlendArg(StrValue);
				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "DestBlendArgAlpha"))
					Blend.DestBlendArgAlpha = StringToBlendArg(StrValue);
				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "BlendOp"))
					Blend.BlendOp = StringToBlendOp(StrValue);
				if (ParamsUtils::TryGetParam(StrValue, BlendDesc, "BlendOpAlpha"))
					Blend.BlendOpAlpha = StringToBlendOp(StrValue);

				Data::CData WriteMask;
				if (ParamsUtils::TryGetParam(WriteMask, BlendDesc, "WriteMask"))
				{
					if (WriteMask.IsVoid())
						Blend.WriteMask = 0;
					else if (WriteMask.IsA<int>())
						Blend.WriteMask = WriteMask.GetValue<int>() & 0x0f;
					else if (WriteMask.IsA<std::string>())
					{
						StrValue = WriteMask.GetValue<std::string>();
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
						Ctx.Log->LogError("Render state '" + ID.ToString() + "' has invalid 'Blend'. 'WriteMask' must be an int or a string, empty or containing r, g, b & a in any combination.");
						return false;
					}
				}
			}
		}

		if (ParamsUtils::TryGetParam(FlagValue, Desc, "AlphaTestEnable"))
			RS.SetFlags(CRenderState::Misc_AlphaTestEnable, FlagValue);
		if (ParamsUtils::TryGetParam(IntValue, Desc, "AlphaTestRef"))
			RS.AlphaTestRef = IntValue;
		if (ParamsUtils::TryGetParam(StrValue, Desc, "AlphaTestFunc"))
			RS.AlphaTestFunc = StringToCmpFunc(StrValue);

		const Data::CData* pClipPlanes;
		if (ParamsUtils::TryGetParam(pClipPlanes, Desc, "ClipPlanes"))
		{
			if (pClipPlanes->IsA<bool>() && pClipPlanes->GetValue<bool>() == false)
			{
				// All clip planes disabled
			}
			else if (pClipPlanes->IsA<int>())
			{
				const int CP = pClipPlanes->GetValue<int>();
				for (int i = 0; i < 5; ++i)
					RS.SetFlags(CRenderState::Misc_ClipPlaneEnable << i, !!(CP & (1 << i)));
			}
			else if (pClipPlanes->IsA<Data::CDataArray>())
			{			
				for (size_t i = 0; i < 5; ++i)
					RS.Flags &= ~(CRenderState::Misc_ClipPlaneEnable << i);
			
				const Data::CDataArray& CP = pClipPlanes->GetValue<Data::CDataArray>();
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
				Ctx.Log->LogError("Render state '" + ID.ToString() + "' has invalid 'ClipPlanes'. Must be 'false', integer bitmask or an array of indices.");
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
			auto ShaderDataIt = Ctx.ShaderCache.find(ShaderID);
			if (ShaderDataIt == Ctx.ShaderCache.cend())
			{
				auto Path = ResolvePathAliases(ShaderID.CStr(), _PathAliases);

				Ctx.Log->LogDebug("Opening shader " + Path.generic_string());

				std::ifstream File(Path, std::ios_base::binary);
				if (!File)
				{
					Ctx.Log->LogError("Can't open shader " + Path.generic_string());
					return false;
				}

				CShaderData ShaderData;
				ReadStream(File, ShaderData.Header);

				// TODO: light count
				// collect max light count of all shaders
				// shader switching is costly, so we don't build per-light-count variations, but instead
				// create the shader with max light count only
				//???how to calculate max light count and whether the shader uses lights at all?
				//???parameter in a shader metafile? will add DEM_MAX_LIGHT_COUNT definition to the compiler + metadata
				//???or find a way to determine light count from shader metadata?

				// Cache metadata bytes
				const size_t MetaSize = ShaderData.Header.MetadataSize;
				if (MetaSize)
				{
					ShaderData.MetaBytes.reset(new char[MetaSize]);
					ShaderData.MetaByteCount = MetaSize;
					if (!File.read(ShaderData.MetaBytes.get(), MetaSize))
					{
						Ctx.Log->LogError("Render state '" + ID.ToString() + "' metadata bytes reading error.");
						return false;
					}
				}

				ShaderDataIt = Ctx.ShaderCache.emplace(ShaderID, std::move(ShaderData)).first;
			}

			const CShaderData& ShaderData = ShaderDataIt->second;

			// TODO: allow DXBC+DXIL for D3D12? Will write DXIL as a state format? What if API supports DXIL only?
			if (RS.ShaderFormatFourCC && RS.ShaderFormatFourCC != ShaderData.Header.Format)
			{
				Ctx.Log->LogError("Render state '" + ID.ToString() + "' has unsupported mix of shader formats.");
				return false;
			}

			RS.ShaderFormatFourCC = ShaderData.Header.Format;

			if (RS.MinFeatureLevel < ShaderData.Header.MinFeatureLevel)
				RS.MinFeatureLevel = ShaderData.Header.MinFeatureLevel;
		}

		RS.IsValid = true;
		return true;
	}

	void SaveRenderState(std::ostream& Stream, const CRenderState& RS)
	{
		// TODO: light count
		//if (!W.Write<U32>(RSRef.MaxLights)) return ERR_IO_WRITE;

		WriteStream(Stream, RS.Flags);

		WriteStream(Stream, RS.DepthBias);
		WriteStream(Stream, RS.DepthBiasClamp);
		WriteStream(Stream, RS.SlopeScaledDepthBias);

		if (RS.Flags & CRenderState::DS_DepthEnable)
			WriteStream<uint8_t>(Stream, RS.DepthFunc);

		if (RS.Flags & CRenderState::DS_StencilEnable)
		{
			WriteStream(Stream, RS.StencilReadMask);
			WriteStream(Stream, RS.StencilWriteMask);
			WriteStream<uint32_t>(Stream, RS.StencilRef);

			WriteStream<uint8_t>(Stream, RS.StencilFrontFace.StencilFailOp);
			WriteStream<uint8_t>(Stream, RS.StencilFrontFace.StencilDepthFailOp);
			WriteStream<uint8_t>(Stream, RS.StencilFrontFace.StencilPassOp);
			WriteStream<uint8_t>(Stream, RS.StencilFrontFace.StencilFunc);

			WriteStream<uint8_t>(Stream, RS.StencilBackFace.StencilFailOp);
			WriteStream<uint8_t>(Stream, RS.StencilBackFace.StencilDepthFailOp);
			WriteStream<uint8_t>(Stream, RS.StencilBackFace.StencilPassOp);
			WriteStream<uint8_t>(Stream, RS.StencilBackFace.StencilFunc);
		}

		for (size_t BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && !(RS.Flags & CRenderState::Blend_Independent)) break;

			const CRenderState::CRTBlend& RTBlend = RS.RTBlend[BlendIdx];
			WriteStream<uint8_t>(Stream, RTBlend.WriteMask);

			if (!(RS.Flags & (CRenderState::Blend_RTBlendEnable << BlendIdx))) continue;

			WriteStream<uint8_t>(Stream, RTBlend.SrcBlendArg);
			WriteStream<uint8_t>(Stream, RTBlend.DestBlendArg);
			WriteStream<uint8_t>(Stream, RTBlend.BlendOp);
			WriteStream<uint8_t>(Stream, RTBlend.SrcBlendArgAlpha);
			WriteStream<uint8_t>(Stream, RTBlend.DestBlendArgAlpha);
			WriteStream<uint8_t>(Stream, RTBlend.BlendOpAlpha);
		}

		WriteStream(Stream, RS.BlendFactorRGBA);
		WriteStream<uint32_t>(Stream, RS.SampleMask);

		WriteStream(Stream, RS.AlphaTestRef);
		WriteStream<uint8_t>(Stream, RS.AlphaTestFunc);

		WriteStream(Stream, RS.VertexShader.ToString());
		WriteStream(Stream, RS.PixelShader.ToString());
		WriteStream(Stream, RS.GeometryShader.ToString());
		WriteStream(Stream, RS.HullShader.ToString());
		WriteStream(Stream, RS.DomainShader.ToString());
	}
};

int main(int argc, const char** argv)
{
	CEffectTool Tool("cf-effect", "DeusExMachina rendering effect compiler", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
