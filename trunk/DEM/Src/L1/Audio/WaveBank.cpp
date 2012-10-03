//#include "WaveBank.h"
//
//#include "WaveResource.h"
//
//namespace Audio
//{
//ImplementRTTI(Audio::CWaveBank, Core::CRefCounted);
//ImplementFactory(Audio::CWaveBank);
//
//nSound3* CWaveBank::CreateSoundObjectFromXmlTable(const nXmlTable& XMLTable, int Row, const nString& ColumnName)
//{
//	nString FileName;
//	FileName.Format("sound:%s", XMLTable.Cell(Row, ColumnName).AsString().Get());
//	n_assert2(DataSrv->FileExists(FileName), FileName.Get());
//
//	nSound3* pSound = nAudioServer3::Instance()->NewSound();
//	pSound->SetName(XMLTable.Cell(Row, "Name").AsString().Get());
//	pSound->SetFilename(FileName);
//	pSound->SetAmbient(XMLTable.Cell(Row, "Ambient").AsBool());
//	pSound->SetStreaming(XMLTable.Cell(Row, "Stream").AsBool());
//	pSound->SetLooping(XMLTable.Cell(Row, "Loop").AsBool());
//	pSound->SetPriority(XMLTable.Cell(Row, "Pri").AsInt());
//	pSound->SetCategory(nAudioServer3::StringToCategory(XMLTable.Cell(Row, "Category").AsString().Get()));
//	pSound->SetNumTracks(XMLTable.Cell(Row, "Tracks").AsInt());
//	pSound->SetMinDist(XMLTable.Cell(Row, "MinDist").AsFloat());
//	pSound->SetMaxDist(XMLTable.Cell(Row, "MaxDist").AsFloat());
//
//	bool IsLoaded = pSound->Load();
//	n_assert2(IsLoaded, FileName.Get());
//
//	return pSound;
//}
////---------------------------------------------------------------------
//
//bool CWaveBank::Open()
//{
//	if (!XMLSheet.Open())
//	{
//		n_error("Audio::CWaveBank::Open(): failed to load '%s'", XMLSheet.GetFilename().Get());
//		return false;
//	}
//
//	nXmlTable& XMLTable = XMLSheet.TableAt(0);
//	n_assert(XMLTable.NumRows() >= 1);
//
//	for (int Row = 1; Row < XMLTable.NumRows(); Row++)
//	{
//		CWaveResource* pResource = CreateResource(XMLTable.Cell(Row, "Name").AsString());
//		n_assert(pResource);
//
//		n_assert(XMLTable.HasColumn("File"));
//		pResource->AddSoundObject(CreateSoundObjectFromXmlTable(XMLTable, Row, "File"));
//
//		if (XMLTable.HasColumn("File2") && XMLTable.Cell(Row, "File2").IsValid())
//		pResource->AddSoundObject(CreateSoundObjectFromXmlTable(XMLTable, Row, "File2"));
//
//		if (XMLTable.HasColumn("File3") && XMLTable.Cell(Row, "File3").IsValid())
//		pResource->AddSoundObject(CreateSoundObjectFromXmlTable(XMLTable, Row, "File3"));
//
//		if (XMLTable.HasColumn("File4") && XMLTable.Cell(Row, "File4").IsValid())
//		pResource->AddSoundObject(CreateSoundObjectFromXmlTable(XMLTable, Row, "File4"));
//
//		pResource->Volume = XMLTable.Cell(Row, "Volume").AsFloat() * 0.01f;
//		AddResource(pResource);
//	}
//	return true;
//}
////---------------------------------------------------------------------
//
//CWaveResource* CWaveBank::FindResource(const nString& Name)
//{
//	n_assert(Name.IsValid());
//	for (int i = 0; i < Resources.Size(); i++)
//		if (Resources[i]->GetName() == Name)
//			return Resources[i];
//	return NULL;
//}
////---------------------------------------------------------------------
//
//CWaveResource* CWaveBank::CreateResource(const nString& Name)
//{
//	n_assert(Name.IsValid());
//	CWaveResource* pNewWave = CWaveResource::Create();
//	pNewWave->SetName(Name);
//	return pNewWave;
//}
////---------------------------------------------------------------------
//
//void CWaveBank::AddResource(CWaveResource* pResource)
//{
//	n_assert(pResource);
//	n_assert(!FindResource(pResource->GetName()));
//	Resources.Append(pResource);
//}
////---------------------------------------------------------------------
//
//} // namespace Audio
