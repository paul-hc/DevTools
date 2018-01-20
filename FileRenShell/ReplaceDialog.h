#ifndef ReplaceDialog_h
#define ReplaceDialog_h
#pragma once

#include "utl/DialogToolBar.h"
#include "utl/HistoryComboBox.h"
#include "utl/LayoutDialog.h"


class CMainRenameDialog;
class CFileSetUi;


class CReplaceDialog : public CLayoutDialog
{
public:
	CReplaceDialog( CMainRenameDialog* pParent );
	virtual ~CReplaceDialog();

	bool Execute( void );
	static std::tstring FormatTooltipMessage( void );
private:
	enum FindType { Find_Text, Find_Characters };

	bool SkipDialog( void ) const;			// skip dialog and execute the last replace directly
	bool ReplaceItems( bool commit = true ) const;
	bool FindMatch( void ) const { return ReplaceItems( false ); }

	// last saved strings in history
	static std::tstring LoadFindWhat( void );
	static std::tstring LoadReplaceWith( void );

	void StoreFindWhatText( const std::tstring& text, const std::vector< std::tstring >* pDestFnames = NULL );
private:
	CMainRenameDialog* m_pParent;
	std::tstring m_findWhat;
	std::tstring m_replaceWith;
	bool m_matchCase;
	FindType m_findType;
private:
	// enum { IDD = IDD_REPLACE_DIALOG };

	CHistoryComboBox m_findWhatCombo;
	CHistoryComboBox m_replaceWithCombo;
	CDialogToolBar m_findToolbar;
	CDialogToolBar m_replaceToolbar;
	std::auto_ptr< CFileSetUi > m_pFileSetUi;
public:
	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK( void );
protected:
	// generated message map
	afx_msg void OnChanged_FindWhat( void );
	afx_msg void OnBnClicked_MatchCase( void );
	afx_msg void OnBnClicked_ClearDestFiles( void );
	afx_msg void OnPickFilename( void );
	afx_msg void OnCopyFindToReplace( void );
	afx_msg void OnFilenamePicked( UINT cmdId );
	afx_msg void OnPickDirPath( void );
	afx_msg void OnDirPathPicked( UINT cmdId );
	afx_msg void OnPickTextTools( void );
	afx_msg void OnFormatTextToolPicked( UINT cmdId );
	afx_msg void OnClearReplace( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ReplaceDialog_h
