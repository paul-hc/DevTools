#ifndef FileChecksumsDialog_h
#define FileChecksumsDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/PathItemListCtrl.h"


class CFileChecksumItem;


class CFileChecksumsDialog : public CLayoutDialog
{
public:
	CFileChecksumsDialog( CWnd* pParent );
	virtual ~CFileChecksumsDialog();
private:
	void SearchForFiles( void );

	// output
	void SetupFileListView( void );
private:
	fs::CPath m_searchPath;

	std::vector<CFileChecksumItem*> m_fileItems;
private:
	// enum { IDD = IDD_FILE_CHECKSUMS_DIALOG };
	enum Column { FileName, DirPath, FileSize, UTL_CRC32, UTL_Elapsed, UTL_ifs_CRC32, UTL_ifs_Elapsed, BoostCFile_CRC32, BoostCFile_Elapsed, Boost_ifs_CRC32, Boost_ifs_Elapsed };

	CItemContentHistoryCombo m_searchPathCombo;
	CPathItemListCtrl m_fileListCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK( void );
	virtual void OnCancel( void ) { OnOK(); }
protected:
	afx_msg void OnBnClicked_FindFiles( void );
	afx_msg void OnBnClicked_CalculateChecksums( void );

	DECLARE_MESSAGE_MAP()
};


#endif // FileChecksumsDialog_h
