#ifndef BuddyControlsDialog_h
#define BuddyControlsDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/PathItemListCtrl.h"


class CFileStateTimedItem;


class CBuddyControlsDialog : public CLayoutDialog
{
public:
	CBuddyControlsDialog( CWnd* pParent );
	virtual ~CBuddyControlsDialog();
private:
	void SearchForFiles( void );

	// output
	void SetupFileListView( void );
private:
	fs::CPath m_searchPath;

	std::vector< CFileStateTimedItem* > m_fileItems;
private:
	// enum { IDD = IDD_BUDDY_CONTROLS_DIALOG };
	enum Column { FilePath, FileSize, Attributes, CRC32, ModifyTime, ChecksumElapsed };

	CSearchPathHistoryCombo m_searchPathCombo;
	CItemContentHistoryCombo m_folderPathCombo;
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


#endif // BuddyControlsDialog_h
