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
		WriteText(Stream, std::to_string(Value.GetValue<float>()));
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
		WriteText(Stream, std::to_string(V.x));
		WriteText(Stream, ", ");
		WriteText(Stream, std::to_string(V.y));
		WriteText(Stream, ", ");
		WriteText(Stream, std::to_string(V.z));
		WriteText(Stream, ", ");
		WriteText(Stream, std::to_string(V.w));
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
				WriteStream(Stream, (float)Value.GetValue<int>());
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<float>())
				WriteStream(Stream, (int)Value.GetValue<float>());
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<bool>())
				WriteStream(Stream, (int)Value.GetValue<bool>());
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

bool SaveParamsByScheme(std::ostream& Stream, const Data::CParams& Params, CStrID SchemeID, const Data::CSchemeSet& SchemeSet)
{
	auto It = SchemeSet.find(SchemeID);
	if (It == SchemeSet.cend()) return false;

	const Data::CDataScheme& Scheme = It->second;

	for (const auto& Rec : Scheme.Records)
	{
		const Data::CData* PrmValue;
		if (!TryGetParam(PrmValue, Params, Rec.ID))
		{
			if (Rec.Default.IsValid()) PrmValue = &Rec.Default;
			else continue;
		}

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
		if (!PrmValue->IsA<Data::CParams>() && !PrmValue->IsA<Data::CDataArray>())
		{
			// Data (self) is not {} or [], write as of type
			if (!WriteDataAsOfType(Stream, *PrmValue, Rec.TypeID, Rec.ComponentCount)) return false;
		}
		else 
		{
			// Data (self) is {} or [], get subscheme if declared
			Data::PDataScheme SubScheme;
			if (Rec.SchemeID.IsValid())
			{
				IPTR Idx = Schemes.FindIndex(Rec.SchemeID);
				if (Idx == INVALID_INDEX) return false;
				SubScheme = Schemes.ValueAt(Idx);
				if (SubScheme.IsNullPtr()) return false;
			}
			else SubScheme = Rec.Scheme;

			if (PrmValue->IsA<Data::PParams>())
			{
				// Data (self) is {}
				const Data::CParams& PrmParams = *PrmValue->GetValue<Data::PParams>();

				// Write element count of self
				U64 CountPos;
				short CountWritten;
				if (Rec.Flags.Is(Data::CDataScheme::WRITE_COUNT))
				{
					CountPos = Stream.GetPosition();
					CountWritten = PrmParams.GetCount();
					if (!Write<short>(CountWritten)) return false;
				}

				if (SubScheme.IsValidPtr() && Rec.Flags.Is(Data::CDataScheme::APPLY_SCHEME_TO_SELF))
				{
					// Apply scheme on self, then fix element count of self
					UPTR Count;
					if (!WriteParamsByScheme(PrmParams, *SubScheme, Schemes, Count)) return false;

					if (Rec.Flags.Is(Data::CDataScheme::WRITE_COUNT) && Count != CountWritten)
					{
						U64 CurrPos = Stream.GetPosition();
						Stream.Seek(CountPos, IO::Seek_Begin);
						if (!Write<short>((short)Count)) return false;
						Stream.Seek(CurrPos, IO::Seek_Begin);
					}
				}
				else for (UPTR j = 0; j < PrmParams.GetCount(); ++j)
				{
					// Apply scheme on children, iterate over them one by one
					const Data::CParam& SubPrm = PrmParams.Get(j);

					// Write key (ID) of current child
					if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_KEYS))
						if (!Write(SubPrm.GetName())) return false;

					// Save data of current child
					if (SubPrm.IsA<Data::PParams>())
					{
						// Current child is {}
						const Data::CParams& SubPrmParams = *SubPrm.GetValue<Data::PParams>();

						// Write element count of current child
						if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT))
						{
							CountPos = Stream.GetPosition();
							CountWritten = SubPrmParams.GetCount();
							if (!Write<short>(CountWritten)) return false;
						}

						if (SubScheme.IsValidPtr())
						{
							// If subscheme is declared, write this child by subscheme and fix its element count
							UPTR Count;
							if (!WriteParamsByScheme(SubPrmParams, *SubScheme, Schemes, Count)) return false;

							if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT) && Count != CountWritten)
							{
								U64 CurrPos = Stream.GetPosition();
								Stream.Seek(CountPos, IO::Seek_Begin);
								if (!Write<short>((short)Count)) return false;
								Stream.Seek(CurrPos, IO::Seek_Begin);
							}
						}
						else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) return false;
					}
					else if (SubPrm.IsA<Data::PDataArray>())
					{
						// Current child is []
						const Data::CDataArray& SubPrmArray = *SubPrm.GetValue<Data::PDataArray>();

						// Write element count of current child
						if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT))
							if (!Write<short>(SubPrmArray.GetCount())) return false;

						// Write array elements one-by-one
						for (UPTR k = 0; k < SubPrmArray.GetCount(); ++k)
							if (!WriteDataAsOfType(SubPrmArray[k], Rec.TypeID, Rec.Flags)) return false;
					}
					else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) return false;
				}
			}
			else // PDataArray
			{
				// Data (self) is []
				const Data::CDataArray& PrmArray = *PrmValue->GetValue<Data::PDataArray>();

				// Write element count of self
				if (Rec.Flags.Is(Data::CDataScheme::WRITE_COUNT))
					if (!Write<short>(PrmArray.GetCount())) return false;

				// Write array elements one-by-one
				for (UPTR j = 0; j < PrmArray.GetCount(); ++j)
				{
					const Data::CData& Element = PrmArray[j];
					if (SubScheme.IsValidPtr() && Element.IsA<Data::PParams>())
					{
						Data::CParams& SubPrmParams = *Element.GetValue<Data::PParams>();

						// Write element count of current child
						//!!!
						// NB: This may cause some problems. Need clarify behaviour of { [ { } ] } structure.
						if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT))
							if (!Write<short>(SubPrmParams.GetCount())) return false;

						// If element is {} and subscheme is declared, save element by subscheme
						if (!WriteParams(SubPrmParams, *SubScheme, Schemes)) return false;
					}
					else if (!WriteDataAsOfType(Element, Rec.TypeID, Rec.Flags)) return false;
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool SaveParamsByScheme(const char* pFileName, const Data::CParams& Params, CStrID SchemeID, const Data::CSchemeSet& SchemeSet)
{
	auto Path = fs::path(pFileName);

	fs::create_directories(Path.parent_path());

	std::ofstream File(Path, std::ios_base::trunc);
	if (!File) return false;

	return SaveParamsByScheme(File, Params, SchemeID, SchemeSet);
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