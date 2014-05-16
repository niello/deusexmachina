//#pragma once
//#ifndef __DEM_L2_WAVE_BANK_H__ //!!!L1!
//#define __DEM_L2_WAVE_BANK_H__
//
//#include <Core/Object.h>
//#include <xml/nxmlspreadsheet.h>
//
//// Holds a number of sound resources. There is usually one wave bank
//// associated with a game level.
//
//#ifdef FindResource
//#undef FindResource
//#endif
//
//namespace Audio
//{
//typedef Ptr<class CWaveResource> PWaveResource;
//
//class CWaveBank: public Core::CObject
//{
//	__DeclareClassNoFactory;
//	__DeclareClass(CWaveBank);
//
//private:
//
//	CArray<PWaveResource>	Resources; //!!!dictionary by name! (or strid???)
//	nXmlSpreadSheet			XMLSheet;
//
//	nSound3* CreateSoundObjectFromXmlTable(const nXmlTable& XMLTable, int Row, const CString& ColumnName);
//
//public:
//
//	CWaveBank(): Resources(128, 128) {}
//	virtual ~CWaveBank() { if (IsOpen()) Close(); }
//
//	bool Open();
//	void Close() { XMLSheet.Close(); }
//	bool IsOpen() const { return XMLSheet.IsOpen(); }
//
//	CWaveResource*	CreateResource(const CString& name);
//	void			AddResource(CWaveResource* pResource);
//	CWaveResource*	FindResource(const CString& name);
//	CWaveResource*	GetResourceAt(int Idx) const { return Resources[Idx]; }
//	int				GetNumResources() const { return Resources.GetCount(); }
//
//	void			SetFilename(const CString& FileName);
//	const CString&	GetFilename() const { return XMLSheet.GetFilename(); }
//};
////---------------------------------------------------------------------
//
//typedef Ptr<CWaveBank> PWaveBank;
//
//inline void CWaveBank::SetFilename(const CString& FileName)
//{
//	if (FileName.IsEmpty()) Sys::Error("Audio::CWaveBank::SetFilename(): got no filename!");
//	XMLSheet.SetFilename(FileName);
//}
////---------------------------------------------------------------------
//
//}
//
//#endif
