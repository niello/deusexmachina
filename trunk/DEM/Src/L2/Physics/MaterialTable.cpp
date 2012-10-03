#include "MaterialTable.h"

#include <Data/DataServer.h>
#include <Data/TTable.h>
#include <Data/XMLDocument.h>

/* <Comment from mangalore>
	Materials used in ODE

	Unit defintions are:
	Density:                Type/m^3
	Force (MegaNewton):     Type*m/s^2
	Momentum:               Type*m/s
	Moment of intertia:     Type*m^2
	Moment of momentum:     Type*m^2/s
	Moment of force:        N*m
	Pressure (MegaPascal):  MN/m^2
	Weight (MegaNewton):    Type*m/s^2
	Work (MegaJoule):       MN*m
	Energy (MegaJoule):     MN*m

	Note: I'm not really sure that this is right... ;-)
*/

namespace Load
{
	bool StringTableFromExcelXML(Data::PXMLDocument Doc, Data::CTTable<nString>& Out,
								 LPCSTR pWorksheetName, bool FirstRowAsColNames, bool FirstColAsRowNames);
}

namespace Physics
{
int CMaterialTable::MtlCount;
nArray<CMaterialTable::CMaterial> CMaterialTable::Materials;
nArray<CMaterialTable::CInteraction> CMaterialTable::Interactions;

void CMaterialTable::Setup()
{
	Data::PXMLDocument Doc = DataSrv->LoadXML("data:tables/materials.xml");

	Data::CTTable<nString> Table;
	n_assert(Load::StringTableFromExcelXML(Doc, Table, "materials", true, false));
	n_assert(Table.GetRowCount() >= 1);

	MtlCount = Table.GetRowCount() - 1; // Second row defines data types
	Materials.SetFixedSize(MtlCount);
	Interactions.SetFixedSize(MtlCount * MtlCount);

	if (MtlCount <= 0) return;

	for (DWORD Row = 1; Row < Table.GetRowCount(); ++Row)
	{
		CMaterial& CMaterial = Materials[Row - 1];
		CMaterial.Name = Table.Cell(CStrID("Name"), Row);
		CMaterial.Density = Table.Cell(CStrID("Density"), Row).AsFloat();
	}

	n_assert(Load::StringTableFromExcelXML(Doc, Table, "friction", false, false));
	n_assert(Table.GetRowCount() == MtlCount + 2 && Table.GetColumnCount() == MtlCount + 1);

	for (DWORD Row = 2; Row < Table.GetRowCount(); ++Row)
	{
		CMaterialType Mtl1 = StringToMaterialType(Table.Cell(0, Row));
		for (DWORD Col = Row - 1; Col < Table.GetColumnCount(); ++Col)
		{
			CMaterialType Mtl2 = StringToMaterialType(Table.Cell(Col, 0));
			Interactions[Mtl1 * MtlCount + Mtl2].Friction =
			Interactions[Mtl2 * MtlCount + Mtl1].Friction = Table.Cell(Col, Row).AsFloat();
		}
	}

	n_assert(Load::StringTableFromExcelXML(Doc, Table, "bouncyness", false, false));
	n_assert(Table.GetRowCount() == MtlCount + 2 && Table.GetColumnCount() == MtlCount + 1);

	for (DWORD Row = 2; Row < Table.GetRowCount(); ++Row)
	{
		CMaterialType Mtl1 = StringToMaterialType(Table.Cell(0, Row));
		for (DWORD Col = Row - 1; Col < Table.GetColumnCount(); ++Col)
		{
			CMaterialType Mtl2 = StringToMaterialType(Table.Cell(Col, 0));
			Interactions[Mtl1 * MtlCount + Mtl2].Bouncyness =
			Interactions[Mtl2 * MtlCount + Mtl1].Bouncyness = Table.Cell(Col, Row).AsFloat();
		}
	}

	if (Load::StringTableFromExcelXML(Doc, Table, "sound", false, false))
	{
		n_assert(Table.GetRowCount() == MtlCount + 2 && Table.GetColumnCount() == MtlCount + 1);

		for (DWORD Row = 2; Row < Table.GetRowCount(); ++Row)
		{
			CMaterialType Mtl1 = StringToMaterialType(Table.Cell(0, Row));
			for (DWORD Col = Row - 1; Col < Table.GetColumnCount(); ++Col)
			{
				CMaterialType Mtl2 = StringToMaterialType(Table.Cell(Col, 0));
				const nString& Sound = Table.Cell(Col, Row);
				if (Sound.IsValid())
				{
					Interactions[Mtl1 * MtlCount + Mtl2].CollisionSound =
					Interactions[Mtl2 * MtlCount + Mtl1].CollisionSound = Sound;
				}
				else
				{
					Interactions[Mtl1 * MtlCount + Mtl2].CollisionSound.Clear();
					Interactions[Mtl2 * MtlCount + Mtl1].CollisionSound.Clear();
				}
			}
		}
	}
}
//---------------------------------------------------------------------

CMaterialType CMaterialTable::StringToMaterialType(const nString& MtlName)
{
	for (int i = 0; i < MtlCount; ++i)
		if (Materials[i].Name == MtlName) return i;
	return InvalidMaterial;
}
//---------------------------------------------------------------------

} // namespace Physics
