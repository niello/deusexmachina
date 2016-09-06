#include "DataServer.h"

#include <Data/HRDParser.h>
#include <Data/Buffer.h>
#include <Data/ParamsUtils.h>
#include <IO/IOServer.h>
#include <IO/HRDWriter.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/Streams/FileStream.h>

namespace Data
{
__ImplementSingleton(Data::CDataServer);

bool CDataServer::LoadDataSchemes(const char* pFileName)
{
	PParams SchemeDescs;
	if (!ParamsUtils::LoadParamsFromHRD(pFileName, SchemeDescs)) FAIL;
	if (SchemeDescs.IsNullPtr()) FAIL;

	for (UPTR i = 0; i < SchemeDescs->GetCount(); ++i)
	{
		const CParam& Prm = SchemeDescs->Get(i);
		if (!Prm.IsA<PParams>()) FAIL;

		IPTR Idx = DataSchemes.FindIndex(Prm.GetName());
		if (Idx != INVALID_INDEX) DataSchemes.RemoveAt(Idx);

		PDataScheme Scheme = n_new(CDataScheme);
		if (!Scheme->Init(*Prm.GetValue<PParams>())) FAIL;
		DataSchemes.Add(Prm.GetName(), Scheme);
	}

	OK;
}
//---------------------------------------------------------------------

}