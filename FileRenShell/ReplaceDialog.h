#ifndef ReplaceDialog_h
#define ReplaceDialog_h
#pragma once

#include "utl/DialogToolBar.h"
#include "utl/HistoryComboBox.h"
#include "utl/LayoutDialog.h"


interface IFileEditor;
class CRenameService;
class CPickDataset;


class CReplaceDialog : public CLayoutDialog
{
public:
	CReplaceDialog( IFileEditor* pParentEditor, const CRenameService* pRenSvc );
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

	void StoreFindWhatText( const std::tstring& text, const std::tstring& commonPrefix );
	void StoreReplaceWithText( const std::tstring& text );
private:
	IFileEditor* m_pParentEditor;
	const CRenameService* m_pRenSvc;

	std::tstring m_findWhat;
	std::tstring m_replaceWith;
	bool m_matchCase;
	FindType m_findType;

	std::auto_ptr< CPickDataset > m_pPickDataset;
private:
	// enum { IDD = IDD_REPLACE_DIALOG };

	CHistoryComboBox m_findWhatCombo;
	CHistoryComboBox m_replaceWithCombo;
	CDialogToolBar m_findToolbar;
	CDialogToolBar m_replaceToolbar;
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
