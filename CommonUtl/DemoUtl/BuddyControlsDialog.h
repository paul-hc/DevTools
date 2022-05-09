#ifndef BuddyControlsDialog_h
#define BuddyControlsDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/TandemControls.h"


class CFileStateTimedItem;
class CResizeFrameStatic;


class CBuddyControlsDialog : public CLayoutDialog
{
public:
	CBuddyControlsDialog( CWnd* pParent );
	virtual ~CBuddyControlsDialog();
private:
	void InitSplitters( void );
	void SearchForFiles( void );

	// output
	void SetupFileListView( void );
private:
	fs::TPatternPath m_searchPath;

	std::vector<CFileStateTimedItem*> m_fileItems;
private:
	// enum { IDD = IDD_BUDDY_CONTROLS_DIALOG };
	enum Column { FilePath, FileSize, Attributes, CRC32, ModifyTime, ChecksumElapsed };

	CSearchPathHistoryCombo m_searchPathCombo;
	CItemContentHistoryCombo m_folderPathCombo;
	CHostToolbarCtrl<CPathItemListCtrl> m_fileListCtrl;
	CProgressCtrl m_progressCtrl;
	CEdit m_selFileEdit;

	std::auto_ptr<CResizeFrameStatic> m_pHorizSplitterFrame;		// embedded inside of vertical splitter
	std::auto_ptr<CResizeFrameStatic> m_pVertSplitterFrame;

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
	afx_msg void OnLvnItemChanged_FileList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // BuddyControlsDialog_h
