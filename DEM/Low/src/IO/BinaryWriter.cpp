#include "BinaryWriter.h"

#include <Data/DataArray.h>
#include <Data/DataScheme.h>
#include <Data/Buffer.h>
#include <Math/Matrix44.h>

namespace IO
{

bool CBinaryWriter::WriteData(const Data::CData& Value)
{
	if (!Write<U8>(Value.GetTypeID())) FAIL;

	if (Value.IsVoid()) OK;
	else if (Value.IsA<bool>()) return Write<bool>(Value);
	else if (Value.IsA<int>()) return Write<int>(Value);
	else if (Value.IsA<float>()) return Write<float>(Value);
	else if (Value.IsA<CString>()) return Write<CString>(Value);
	else if (Value.IsA<CStrID>()) return Write<CStrID>(Value);
	else if (Value.IsA<vector3>()) return Write<vector3>(Value);
	else if (Value.IsA<vector4>()) return Write<vector4>(Value);
	else if (Value.IsA<matrix44>()) return Write<matrix44>(Value);
	else if (Value.IsA<Data::PParams>()) return Write<Data::PParams>(Value);
	else if (Value.IsA<Data::PDataArray>()) return Write<Data::PDataArray>(Value);
	else if (Value.IsA<Data::CBuffer>())  return Write<Data::CBuffer>(Value);
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

template<> bool CBinaryWriter::Write<Data::CDataArray>(const Data::CDataArray& Value)
{
	if (!Write<U16>(Value.GetCount())) FAIL;
	for (UPTR i = 0; i < Value.GetCount(); ++i)
		if (!WriteData(Value[i])) FAIL;
	OK;
}
//---------------------------------------------------------------------

template<> bool CBinaryWriter::Write<Data::CBuffer>(const Data::CBuffer& Value)
{
	return Write<U32>(Value.GetSize()) && (!Value.GetSize() || Stream.Write(Value.GetPtr(), Value.GetSize()));
}
//---------------------------------------------------------------------

bool CBinaryWriter::WriteParamsByScheme(const Data::CParams& Value,
										const Data::CDataScheme& Scheme,
										const CDict<CStrID, Data::PDataScheme>& Schemes,
										UPTR& Written)
{
	Written = 0;

	for (UPTR i = 0; i < Scheme.Records.GetCount(); ++i)
	{
		const Data::CDataScheme::CRecord& Rec = Scheme.Records[i];

		const Data::CData* PrmValue;
		if (!Value.Get(PrmValue, Rec.ID))
		{
			if (Rec.Default.IsValid()) PrmValue = &Rec.Default;
			else continue;
		}

		++Written;

		// Write Key or FourCC of current param
		if (Rec.FourCC.IsValid())
		{
			if (!Write<int>(Rec.FourCC.Code)) FAIL;
		}
		else if (Rec.Flags.Is(Data::CDataScheme::WRITE_KEY))
		{
			if (!Write(Rec.ID)) FAIL;
		}

		// Write data (self)
		if (!PrmValue->IsA<Data::PParams>() && !PrmValue->IsA<Data::PDataArray>())
		{
			// Data (self) is not {} or [], write as of type
			if (!WriteDataAsOfType(*PrmValue, Rec.TypeID, Rec.Flags)) FAIL;
		}
		else 
		{
			// Data (self) is {} or [], get subscheme if declared
			Data::PDataScheme SubScheme;
			if (Rec.SchemeID.IsValid())
			{
				IPTR Idx = Schemes.FindIndex(Rec.SchemeID);
				if (Idx == INVALID_INDEX) FAIL;
				SubScheme = Schemes.ValueAt(Idx);
				if (SubScheme.IsNullPtr()) FAIL;
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
					if (!Write<short>(CountWritten)) FAIL;
				}

				if (SubScheme.IsValidPtr() && Rec.Flags.Is(Data::CDataScheme::APPLY_SCHEME_TO_SELF))
				{
					// Apply scheme on self, then fix element count of self
					UPTR Count;
					if (!WriteParamsByScheme(PrmParams, *SubScheme, Schemes, Count)) FAIL;

					if (Rec.Flags.Is(Data::CDataScheme::WRITE_COUNT) && Count != CountWritten)
					{
						U64 CurrPos = Stream.GetPosition();
						Stream.Seek(CountPos, IO::Seek_Begin);
						if (!Write<short>((short)Count)) FAIL;
						Stream.Seek(CurrPos, IO::Seek_Begin);
					}
				}
				else for (UPTR j = 0; j < PrmParams.GetCount(); ++j)
				{
					// Apply scheme on children, iterate over them one by one
					const Data::CParam& SubPrm = PrmParams.Get(j);

					// Write key (ID) of current child
					if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_KEYS))
						if (!Write(SubPrm.GetName())) FAIL;

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
							if (!Write<short>(CountWritten)) FAIL;
						}

						if (SubScheme.IsValidPtr())
						{
							// If subscheme is declared, write this child by subscheme and fix its element count
							UPTR Count;
							if (!WriteParamsByScheme(SubPrmParams, *SubScheme, Schemes, Count)) FAIL;

							if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT) && Count != CountWritten)
							{
								U64 CurrPos = Stream.GetPosition();
								Stream.Seek(CountPos, IO::Seek_Begin);
								if (!Write<short>((short)Count)) FAIL;
								Stream.Seek(CurrPos, IO::Seek_Begin);
							}
						}
						else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) FAIL;
					}
					else if (SubPrm.IsA<Data::PDataArray>())
					{
						// Current child is []
						const Data::CDataArray& SubPrmArray = *SubPrm.GetValue<Data::PDataArray>();

						// Write element count of current child
						if (Rec.Flags.Is(Data::CDataScheme::WRITE_CHILD_COUNT))
							if (!Write<short>(SubPrmArray.GetCount())) FAIL;

						// Write array elements one-by-one
						for (UPTR k = 0; k < SubPrmArray.GetCount(); ++k)
							if (!WriteDataAsOfType(SubPrmArray[k], Rec.TypeID, Rec.Flags)) FAIL;
					}
					else if (!WriteDataAsOfType(SubPrm.GetRawValue(), Rec.TypeID, Rec.Flags)) FAIL;
				}
			}
			else // PDataArray
			{
				// Data (self) is []
				const Data::CDataArray& PrmArray = *PrmValue->GetValue<Data::PDataArray>();

				// Write element count of self
				if (Rec.Flags.Is(Data::CDataScheme::WRITE_COUNT))
					if (!Write<short>(PrmArray.GetCount())) FAIL;

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
							if (!Write<short>(SubPrmParams.GetCount())) FAIL;

						// If element is {} and subscheme is declared, save element by subscheme
						if (!WriteParams(SubPrmParams, *SubScheme, Schemes)) FAIL;
					}
					else if (!WriteDataAsOfType(Element, Rec.TypeID, Rec.Flags)) FAIL;
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CBinaryWriter::WriteDataAsOfType(const Data::CData& Value, int TypeID, Data::CFlags Flags)
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