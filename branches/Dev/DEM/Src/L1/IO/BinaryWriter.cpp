#include "BinaryWriter.h"

#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Data/Buffer.h>

namespace IO
{
using namespace Data;

bool CBinaryWriter::WriteData(const CData& Value)
{
	if (!Write<char>(Value.GetTypeID())) FAIL;

	if (Value.IsVoid()) OK;
	else if (Value.IsA<bool>()) return Write<bool>(Value);
	else if (Value.IsA<int>()) return Write<int>(Value);
	else if (Value.IsA<float>()) return Write<float>(Value);
	else if (Value.IsA<CString>()) return Write<CString>(Value);
	else if (Value.IsA<CStrID>()) return Write<CStrID>(Value);
	else if (Value.IsA<vector3>()) return Write<vector3>(Value);
	else if (Value.IsA<vector4>()) return Write<vector4>(Value);
	else if (Value.IsA<matrix44>()) return Write<matrix44>(Value);
	else if (Value.IsA<PParams>()) return Write<PParams>(Value);
	else if (Value.IsA<PDataArray>()) return Write<PDataArray>(Value);
	else if (Value.IsA<CBuffer>())  return Write<CBuffer>(Value);
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

template<> bool CBinaryWriter::Write<CDataArray>(const CDataArray& Value)
{
	if (!Write<short>(Value.GetCount())) FAIL;
	for (int i = 0; i < Value.GetCount(); i++)
		if (!WriteData(Value[i])) FAIL;
	OK;
}
//---------------------------------------------------------------------

template<> bool CBinaryWriter::Write<CBuffer>(const CBuffer& Value)
{
	return Write<int>(Value.GetSize()) && (!Value.GetSize() || Stream.Write(Value.GetPtr(), Value.GetSize()));
}
//---------------------------------------------------------------------

bool CBinaryWriter::WriteParamsByScheme(const CParams& Value, const CDataScheme& Scheme, DWORD& Written)
{
	Written = 0;

	for (int i = 0; i < Scheme.Records.GetCount(); ++i)
	{
		const CDataScheme::CRecord& Rec = Scheme.Records[i];

		const CData* PrmValue;
		if (!Value.Get(PrmValue, Rec.ID))
		{
			if (Rec.Default.IsValid()) PrmValue = &Rec.Default;
			else continue;
		}

		++Written;

		// Write Key of FourCC of current param
		if (Rec.FourCC.IsValid())
		{
			if (!Write<int>(Rec.FourCC.Code)) FAIL;
		}
		else if (Rec.Flags.Is(CDataScheme::WRITE_KEY))
		{
			if (!Write(Rec.ID)) FAIL;
		}

		// Write data (self)
		if (!PrmValue->IsA<PParams>() && !PrmValue->IsA<PDataArray>())
		{
			// Data (self) is not {} or [], write as of type
			if (!WriteDataAsOfType(*PrmValue, Rec.TypeID, Rec.Flags)) FAIL;
		}
		else 
		{
			// Data (self) is {} or [], get subscheme if declared
			PDataScheme SubScheme;
			if (Rec.SchemeID.IsValid())
			{
				SubScheme = DataSrv->GetDataScheme(Rec.SchemeID);
				if (SubScheme.IsNullPtr()) FAIL;
			}
			else SubScheme = Rec.Scheme;
			
			if (PrmValue->IsA<PParams>())
			{
				// Data (self) is {}
				const CParams& PrmParams = *PrmValue->GetValue<PParams>();

				// Write element count of self
				DWORD CountPos;
				short CountWritten;
				if (Rec.Flags.Is(CDataScheme::WRITE_COUNT))
				{
					CountPos = Stream.GetPosition();
					CountWritten = PrmParams.GetCount();
					if (!Write<short>(CountWritten)) FAIL;
				}

				if (SubScheme.IsValidPtr() && Rec.Flags.Is(CDataScheme::APPLY_SCHEME_TO_SELF))
				{
					// Apply scheme on self, then fix element count of self
					DWORD Count;
					if (!WriteParamsByScheme(PrmParams, *SubScheme, Count)) FAIL;

					if (Rec.Flags.Is(CDataScheme::WRITE_COUNT) && Count != CountWritten)
					{
						DWORD CurrPos = Stream.GetPosition();
						Stream.Seek(CountPos, IO::Seek_Begin);
						if (!Write<short>((short)Count)) FAIL;
						Stream.Seek(CurrPos, IO::Seek_Begin);
					}
				}
				else for (int j = 0; j < PrmParams.GetCount(); ++j)
				{
					// Apply scheme on children, iterate over them one by one
					const CParam& SubPrm = PrmParams.Get(j);

					// Write key (ID) of current child
					if (Rec.Flags.Is(CDataScheme::WRITE_CHILD_KEYS))
						if (!Write(SubPrm.GetName())) FAIL;

					// Save data of current child
					if (SubPrm.IsA<PParams>())
					{
						// Current child is {}
						const CParams& SubPrmParams = *SubPrm.GetValue<PParams>();

						// Write element count of current child
						if (Rec.Flags.Is(CDataScheme::WRITE_CHILD_COUNT))
						{
							CountPos = Stream.GetPosition();
							CountWritten = SubPrmParams.GetCount();
							if (!Write<short>(CountWritten)) FAIL;
						}

						if (SubScheme.IsValidPtr())
						{
							// If subscheme is declared, write this child by subscheme and fix its element count
							DWORD Count;
							if (!WriteParamsByScheme(SubPrmParams, *SubScheme, Count)) FAIL;

							if (Rec.Flags.Is(CDataScheme::WRITE_CHILD_COUNT) && Count != CountWritten)
							{
								DWORD CurrPos = Stream.GetPosition();
								Stream.Seek(CountPos, IO::Seek_Begin);
								if (!Write<short>((short)Count)) FAIL;
								Stream.Seek(CurrPos, IO::Seek_Begin);
							}
						}
						else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) FAIL;
					}
					else if (SubPrm.IsA<PDataArray>())
					{
						// Current child is []
						const CDataArray& SubPrmArray = *SubPrm.GetValue<PDataArray>();

						// Write element count of current child
						if (Rec.Flags.Is(CDataScheme::WRITE_CHILD_COUNT))
							if (!Write<short>(SubPrmArray.GetCount())) FAIL;

						// Write array elements one-by-one
						for (int k = 0; k < SubPrmArray.GetCount(); ++k)
							if (!WriteDataAsOfType(SubPrmArray[k], Rec.TypeID, Rec.Flags)) FAIL;
					}
					else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) FAIL;
				}
			}
			else // PDataArray
			{
				// Data (self) is []
				const CDataArray& PrmArray = *PrmValue->GetValue<PDataArray>();

				// Write element count of self
				if (Rec.Flags.Is(CDataScheme::WRITE_COUNT))
					if (!Write<short>(PrmArray.GetCount())) FAIL;

				// Write array elements one-by-one
				for (int j = 0; j < PrmArray.GetCount(); ++j)
				{
					const CData& Element = PrmArray[j];
					if (SubScheme.IsValidPtr() && Element.IsA<PParams>())
					{
						CParams& SubPrmParams = *Element.GetValue<PParams>();

						// Write element count of current child
						//!!!
						// NB: This may cause some problems. Need clarify behaviour of { [ { } ] } structure.
						if (Rec.Flags.Is(CDataScheme::WRITE_CHILD_COUNT))
							if (!Write<short>(SubPrmParams.GetCount())) FAIL;

						// If element is {} and subscheme is declared, save element by subscheme
						if (!WriteParams(SubPrmParams, *SubScheme)) FAIL;
					}
					else if (!WriteDataAsOfType(Element, Rec.TypeID, Rec.Flags)) FAIL;
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CBinaryWriter::WriteDataAsOfType(const CData& Value, int TypeID, CFlags Flags)
{
	if (TypeID == TVoid) return Write(Value);
	else
	{
		if (Value.GetTypeID() != TypeID)
		{
			// Conversion cases //!!!Need CType conversion!
			if (TypeID == DATA_TYPE_ID(float) && Value.IsA<int>())
				return Write((float)Value.GetValue<int>());
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<float>())
				return Write((int)Value.GetValue<float>());
			else if (TypeID == DATA_TYPE_ID(int) && Value.IsA<bool>())
				return Write((int)Value.GetValue<bool>());
			else if (TypeID == DATA_TYPE_ID(CStrID) && Value.IsA<CString>())
				return Write(Value.GetValue<CString>());
			else if (TypeID == DATA_TYPE_ID(CString) && Value.IsA<CStrID>())
				return Write(Value.GetValue<CStrID>());
			else FAIL;
		}

		if (TypeID == DATA_TYPE_ID(bool)) return Write(Value.GetValue<bool>());
		else if (TypeID == DATA_TYPE_ID(int)) return Write(Value.GetValue<int>());
		else if (TypeID == DATA_TYPE_ID(float)) return Write(Value.GetValue<float>());
		else if (TypeID == DATA_TYPE_ID(CString)) return Write(Value.GetValue<CString>());
		else if (TypeID == DATA_TYPE_ID(CStrID)) return Write(Value.GetValue<CStrID>());
		else if (TypeID == DATA_TYPE_ID(vector3)) return Write(Value.GetValue<vector3>());
		else if (TypeID == DATA_TYPE_ID(vector4)) return Write(Value.GetValue<vector4>());
	}

	FAIL;
}
//---------------------------------------------------------------------

}