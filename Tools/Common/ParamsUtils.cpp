#include "ParamsUtils.h"
#include <Utils.h>
#include <HRDParser.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace ParamsUtils
{

bool LoadParamsFromHRD(const char* pFileName, Data::CParams& OutParams)
{
	std::vector<char> In;
	if (!ReadAllFile(pFileName, In, false)) return false;

	Data::CHRDParser Parser;
	return Parser.ParseBuffer(In.data(), In.size(), OutParams);
}
//---------------------------------------------------------------------

/*
bool LoadParamsFromPRM(const char* pFileName, Data::CParams& OutParams)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_READ)) return nullptr;
	IO::CBinaryReader Reader(*File);

	Data::PParams Params = n_new(Data::CParams);
	if (!Reader.ReadParams(*Params)) return false;
	OutParams = Params;
	return true;
}
//---------------------------------------------------------------------

bool LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName, Data::CParams& OutParams)
{
	Data::PParams Main;
	if (!ParamsUtils::LoadParamsFromPRM(std::string(pRootPath) + pRelativeFileName, Main)) return false;
	if (Main.IsNullPtr()) return false;

	std::string BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		n_assert(BaseName != pRelativeFileName);
		if (!LoadDescFromPRM(pRootPath, BaseName + ".prm", OutParams)) return false;
		OutParams->Merge(*Main, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else OutParams = n_new(Data::CParams(*Main));

	return true;
}
//---------------------------------------------------------------------
*/

bool LoadSchemes(const char* pFileName, Data::CSchemeSet& OutSchemes)
{
	Data::CParams SchemeDescs;
	if (!ParamsUtils::LoadParamsFromHRD(pFileName, SchemeDescs)) return false;
	if (SchemeDescs.empty()) return false;

	for (const auto& Prm : SchemeDescs)
	{
		if (!Prm.second.IsA<Data::CParams>()) return false;

		Data::CDataScheme Scheme;
		if (!Scheme.Init(Prm.second.GetValue<Data::CParams>())) return false;
		OutSchemes[Prm.first] = std::move(Scheme);
	}

	return true;
}
//---------------------------------------------------------------------

static std::string GetIndentation(size_t Depth, const std::string& Element = "\t")
{
	std::string Indentation;
	Indentation.reserve(Element.length() * Depth);
	for (size_t i = 0; i < Depth; ++i)
		Indentation += Element;
	return Indentation;
}
//---------------------------------------------------------------------

static bool WriteParamAsHRD(std::ostream& Stream, const Data::CParam& Value, size_t Depth);

static std::string ToStringNoTrailingZeroes(float Value)
{
	std::string Str = std::to_string(Value);
	const auto DotPos = Str.find_first_of('.');
	if (DotPos != std::string::npos)
	{
		auto BeforeZeroPos = Str.find_last_not_of('0');
		if (BeforeZeroPos == DotPos)
			++BeforeZeroPos;
		Str.erase(BeforeZeroPos + 1, std::string::npos);
	}
	return Str;
}
//---------------------------------------------------------------------

static bool WriteDataAsHRD(std::ostream& Stream, const Data::CData& Value, size_t Depth)
{
	if (Value.IsVoid()) WriteText(Stream, "null");
	else if (Value.IsA<bool>())
	{
		if (Value.GetValue<bool>())
			WriteText(Stream, "true");
		else
			WriteText(Stream, "false");
	}
	else if (Value.IsA<int>())
	{
		WriteText(Stream, std::to_string(Value.GetValue<int>()));
	}
	else if (Value.IsA<float>())
	{
		WriteText(Stream, ToStringNoTrailingZeroes(Value.GetValue<float>()));
	}
	else if (Value.IsA<std::string>())
	{
		WriteText(Stream, "\"");
		WriteText(Stream, Value.GetValue<std::string>());
		WriteText(Stream, "\"");
	}
	else if (Value.IsA<CStrID>())
	{
		//!!!correctly serialize \n, \t etc!
		WriteText(Stream, "'");
		WriteText(Stream, Value.GetValue<CStrID>());
		WriteText(Stream, "'");
	}
	else if (Value.IsA<vector4>())
	{
		const vector4& V = Value.GetValue<vector4>();
		WriteText(Stream, "(");
		WriteText(Stream, ToStringNoTrailingZeroes(V.x));
		WriteText(Stream, ", ");
		WriteText(Stream, ToStringNoTrailingZeroes(V.y));
		WriteText(Stream, ", ");
		WriteText(Stream, ToStringNoTrailingZeroes(V.z));
		WriteText(Stream, ", ");
		WriteText(Stream, ToStringNoTrailingZeroes(V.w));
		WriteText(Stream, ")");
	}
	else if (Value.IsA<Data::CDataArray>())
	{
		const std::string Indentation = GetIndentation(Depth);
		WriteText(Stream, Indentation);
		WriteText(Stream, "[\n");

		++Depth;
		const std::string InnerIndentation = GetIndentation(Depth);

		const Data::CDataArray& A = Value.GetValue<Data::CDataArray>();
		for (size_t i = 0; i < A.size(); ++i)
		{
			const Data::CData& Elm = A[i];
			if (!Elm.IsA<Data::CParams>() && !Elm.IsA<Data::CDataArray>())
				WriteText(Stream, InnerIndentation);
			if (!WriteDataAsHRD(Stream, Elm, Depth)) return false;

			if (i < A.size() - 1)
				WriteText(Stream, ",\n");
			else
				WriteText(Stream, "\n");
		}

		--Depth;
		WriteText(Stream, Indentation);
		WriteText(Stream, "]");
	}
	else if (Value.IsA<Data::CParams>())
	{
		const std::string Indentation = GetIndentation(Depth);
		WriteText(Stream, Indentation);
		WriteText(Stream, "{\n");

		++Depth;
		const std::string InnerIndentation = GetIndentation(Depth);

		const Data::CParams& P = Value.GetValue<Data::CParams>();
		for (const auto& Param : P)
		{
			WriteText(Stream, InnerIndentation);
			if (!WriteParamAsHRD(Stream, Param, Depth)) return false;
		}

		--Depth;
		WriteText(Stream, Indentation);
		WriteText(Stream, "}");
	}
	else
	{
		assert(false && "WriteDataAsHRD() > unknown type!");
		return false;
	}

	return true;
}
//---------------------------------------------------------------------

static bool WriteParamAsHRD(std::ostream& Stream, const Data::CParam& Value, size_t Depth)
{
	WriteText(Stream, Value.first);

	if (Value.second.IsA<Data::CParams>() || Value.second.IsA<Data::CDataArray>())
		WriteText(Stream, "\n");
	else
		WriteText(Stream, " = ");

	if (!WriteDataAsHRD(Stream, Value.second, Depth)) return false;

	WriteText(Stream, "\n");
	return true;
}
//---------------------------------------------------------------------

static bool WriteParamsAsHRD(std::ostream& Stream, const Data::CParams& Value, size_t Depth)
{
	for (const auto Param : Value)
	{
		if (!WriteParamAsHRD(Stream, Param, Depth)) return false;
		if (Param.second.IsA<Data::CParams>())
			WriteText(Stream, "\n");
	}
	return true;
}
//---------------------------------------------------------------------

bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params)
{
	auto Path = fs::path(pFileName);

	fs::create_directories(Path.parent_path());

	std::ofstream File(Path, std::ios_base::trunc);
	if (!File) return false;

	return WriteParamsAsHRD(File, Params, 0);
}
//---------------------------------------------------------------------

/*
bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_WRITE)) return false;
	IO::CBinaryWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------
*/

static bool WriteDataAsOfType(std::ostream& Stream, const Data::CData& Value, int TypeID, uint8_t ComponentCount)
{
	if (TypeID == INVALID_TYPE_ID)
	{
		WriteStream(Stream, Value);
	}
	else
	{
		if (Value.GetTypeID() != TypeID)
		{
			// Conversion cases //!!!Need CType conversion!
			if (TypeID == DATA_TYPE_ID(float) && Value.IsA<int>())
				WriteStream(Stream, static_cast<float>(Value.GetValue<int>()));
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<float>())
				WriteStream(Stream, static_cast<int>(Value.GetValue<float>()));
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<bool>())
				WriteStream(Stream, static_cast<int>(Value.GetValue<bool>()));
			else if (TypeID == DATA_TYPE_ID(CStrID) && Value.IsA<std::string>())
				WriteStream(Stream, Value.GetValue<std::string>());
			else if (TypeID == DATA_TYPE_ID(std::string) && Value.IsA<CStrID>())
				WriteStream(Stream, Value.GetValue<CStrID>());
			else return false;
		}

		if (TypeID == DATA_TYPE_ID(bool))
			WriteStream(Stream, Value.GetValue<bool>());
		else if (TypeID == DATA_TYPE_ID(int))
			WriteStream(Stream, Value.GetValue<int>());
		else if (TypeID == DATA_TYPE_ID(float))
			WriteStream(Stream, Value.GetValue<float>());
		else if (TypeID == DATA_TYPE_ID(std::string))
			WriteStream(Stream, Value.GetValue<std::string>());
		else if (TypeID == DATA_TYPE_ID(CStrID))
			WriteStream(Stream, Value.GetValue<CStrID>());
		else if (TypeID == DATA_TYPE_ID(vector4))
		{
			assert(ComponentCount >= 1 && ComponentCount <= 4);
			Stream.write(reinterpret_cast<const char*>(&Value.GetValue<vector4>().x), ComponentCount * sizeof(float));
		}
		else return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool SaveParamsByScheme(std::ostream& Stream, const Data::CParams& Params, const Data::CDataScheme& Scheme, const Data::CSchemeSet& SchemeSet, size_t* pElementsWritten)
{
	if (pElementsWritten) *pElementsWritten = 0;

	for (const auto& Rec : Scheme.Records)
	{
		const Data::CData* pPrmValue;
		if (!TryGetParam(pPrmValue, Params, Rec.ID))
		{
			if (Rec.Default.IsValid()) pPrmValue = &Rec.Default;
			else continue;
		}

		if (pElementsWritten) ++(*pElementsWritten);

		// Write Key or FourCC of current param
		if (Rec.FourCC)
		{
			WriteStream(Stream, Rec.FourCC);
		}
		else if (Rec.WriteKey)
		{
			WriteStream(Stream, Rec.ID);
		}

		// Write data (self)
		if (!pPrmValue->IsA<Data::CParams>() && !pPrmValue->IsA<Data::CDataArray>())
		{
			// Data (self) is not {} or [], write as of type
			if (!WriteDataAsOfType(Stream, *pPrmValue, Rec.TypeID, Rec.ComponentCount)) return false;
		}
		else 
		{
			// Data (self) is {} or [], get subscheme if declared
			const Data::CDataScheme* pSubScheme;
			if (Rec.SchemeID)
			{
				auto SubIt = SchemeSet.find(Rec.SchemeID);
				if (SubIt == SchemeSet.cend()) return false;
				pSubScheme = &SubIt->second;
			}
			else pSubScheme = Rec.Scheme.get();

			if (pPrmValue->IsA<Data::CParams>())
			{
				// Data (self) is {}
				const Data::CParams& PrmParams = pPrmValue->GetValue<Data::CParams>();

				// Write element count of self
				std::streampos CountPos;
				size_t CountWritten;
				if (Rec.WriteCount)
				{
					CountPos = Stream.tellp();
					CountWritten = PrmParams.size();
					WriteStream(Stream, static_cast<uint16_t>(CountWritten));
				}

				if (pSubScheme && Rec.ApplySchemeToSelf)
				{
					// Apply scheme on self
					size_t Count;
					if (!SaveParamsByScheme(Stream, PrmParams, *pSubScheme, SchemeSet, &Count)) return false;

					// Fix element count of self
					if (Rec.WriteCount && Count != CountWritten)
					{
						auto CurrPos = Stream.tellp();
						Stream.seekp(CountPos, std::ios_base::beg);
						WriteStream(Stream, static_cast<uint16_t>(Count));
						Stream.seekp(CurrPos, std::ios_base::beg);
					}
				}
				else
				{
					// Apply scheme on children, iterate over them one by one
					for (const auto& SubPrm : PrmParams)
					{
						// Write key (ID) of current child
						if (Rec.WriteChildKeys)
							WriteStream(Stream, SubPrm.first);

						// Save data of current child
						if (SubPrm.second.IsA<Data::CParams>())
						{
							// Current child is {}
							const Data::CParams& SubPrmParams = SubPrm.second.GetValue<Data::CParams>();

							// Write element count of current child
							if (Rec.WriteChildCount)
							{
								CountPos = Stream.tellp();
								CountWritten = PrmParams.size();
								WriteStream(Stream, static_cast<uint16_t>(CountWritten));
							}

							if (pSubScheme)
							{
								// FIXME: duplicated code! if all subschemes are processed with this,
								// move count writing into SaveParamsByScheme?

								// If subscheme is declared, write this child by subscheme and fix its element count
								size_t Count;
								if (!SaveParamsByScheme(Stream, SubPrmParams, *pSubScheme, SchemeSet, &Count)) return false;

								if (Rec.WriteCount && Count != CountWritten)
								{
									auto CurrPos = Stream.tellp();
									Stream.seekp(CountPos, std::ios_base::beg);
									WriteStream(Stream, static_cast<uint16_t>(Count));
									Stream.seekp(CurrPos, std::ios_base::beg);
								}
							}
							else
							{
								if (!WriteDataAsOfType(Stream, SubPrm.second, Rec.TypeID, Rec.ComponentCount)) return false;
							}
						}
						else if (SubPrm.second.IsA<Data::CDataArray>())
						{
							// Current child is []
							const Data::CDataArray& SubPrmArray = SubPrm.second.GetValue<Data::CDataArray>();

							// Write element count of current child
							if (Rec.WriteChildCount)
								WriteStream(Stream, static_cast<uint16_t>(SubPrmArray.size()));

							// Write array elements one-by-one
							for (const auto& Element : SubPrmArray)
								if (!WriteDataAsOfType(Stream, Element, Rec.TypeID, Rec.ComponentCount)) return false;
						}
						else
						{
							if (!WriteDataAsOfType(Stream, SubPrm.second, Rec.TypeID, Rec.ComponentCount)) return false;
						}
					}
				}
			}
			else // PDataArray
			{
				// Data (self) is []
				const Data::CDataArray& PrmArray = pPrmValue->GetValue<Data::CDataArray>();

				// Write element count of self
				if (Rec.WriteCount)
					WriteStream(Stream, static_cast<uint16_t>(PrmArray.size()));

				// Write array elements one-by-one
				for (const auto& Element : PrmArray)
				{
					if (pSubScheme && Element.IsA<Data::CParams>())
					{
						const Data::CParams& SubPrmParams = Element.GetValue<Data::CParams>();

						// Write element count of current child
						//!!!
						// NB: This may cause some problems. Need clarify behaviour of { [ { } ] } structure.
						if (Rec.WriteChildCount)
							WriteStream(Stream, static_cast<uint16_t>(SubPrmParams.size()));

						// If element is {} and subscheme is declared, save element by subscheme
						if (!SaveParamsByScheme(Stream, SubPrmParams, *pSubScheme, SchemeSet)) return false;
					}
					else
					{
						if (!WriteDataAsOfType(Stream, Element, Rec.TypeID, Rec.ComponentCount)) return false;
					}
				}
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool SaveParamsByScheme(const char* pFileName, const Data::CParams& Params, CStrID SchemeID, const Data::CSchemeSet& SchemeSet)
{
	auto It = SchemeSet.find(SchemeID);
	if (It == SchemeSet.cend()) return false;

	const Data::CDataScheme& Scheme = It->second;

	auto Path = fs::path(pFileName);

	fs::create_directories(Path.parent_path());

	std::ofstream File(Path, std::ios_base::trunc);
	if (!File) return false;

	return SaveParamsByScheme(File, Params, Scheme, SchemeSet);
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(Data::CData& Out, const Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend()) return false;
	Out = It->second;
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CData*& Out, const Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second;
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CParams*& Out, const Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend() || !It->second.IsA<Data::CParams>())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second.GetValue<Data::CParams>();
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CDataArray*& Out, const Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.cbegin(), Params.cend(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.cend() || !It->second.IsA<Data::CDataArray>())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second.GetValue<Data::CDataArray>();
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CData*& Out, Data::CParams& Params, const char* pKey)
{
	return TryGetParam(Out, static_cast<const Data::CParams&>(Params), pKey);
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CParams*& Out, Data::CParams& Params, const char* pKey)
{
	return TryGetParam(Out, static_cast<const Data::CParams&>(Params), pKey);
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(const Data::CDataArray*& Out, Data::CParams& Params, const char* pKey)
{
	return TryGetParam(Out, static_cast<const Data::CParams&>(Params), pKey);
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(Data::CData*& Out, Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.begin(), Params.end(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.end())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second;
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(Data::CParams*& Out, Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.begin(), Params.end(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.end() || !It->second.IsA<Data::CParams>())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second.GetValue<Data::CParams>();
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(Data::CDataArray*& Out, Data::CParams& Params, const char* pKey)
{
	auto It = std::find_if(Params.begin(), Params.end(), [Key = CStrID(pKey)](const auto& Param) { return Param.first == Key; });
	if (It == Params.end() || !It->second.IsA<Data::CDataArray>())
	{
		Out = nullptr;
		return false;
	}
	Out = &It->second.GetValue<Data::CDataArray>();
	return true;
}
//---------------------------------------------------------------------

template<>
const bool TryGetParam(Data::CData& Out, Data::CParamsSorted& Params, const char* pKey)
{
	auto It = Params.find(CStrID(pKey));
	if (It == Params.cend()) return false;
	Out = It->second;
	return true;
}
//---------------------------------------------------------------------

}