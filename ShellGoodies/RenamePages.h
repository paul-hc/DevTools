#ifndef RenamePages_h
#define RenamePages_h
#pragma once

#include "utl/ISubject.h"
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/SyncScrolling.h"
#include "utl/UI/TextEditor.h"
#include "utl/UI/ThemeStatic.h"
#include "Application_fwd.h"


class CFileModel;
class CRenameFilesDialog;
class CRenameItem;


interface IRenamePage : public utl::IObserver
{
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem ) = 0;
	virtual void InvalidateFiles( void ) {}
};


abstract class CBaseRenamePage : public CLayoutPropertyPage
	, public CInternalChange
	, public IRenamePage
{
protected:
	CBaseRenamePage( UINT templateId, CRenameFilesDialog* pParentDlg )
		: CLayoutPropertyPage( templateId )
		, m_pParentDlg( pParentDlg )
	{
		ASSERT_PTR( m_pParentDlg );
	}
protected:
	CRenameFilesDialog* m_pParentDlg;
};


abstract class CBaseRenameListPage : public CBaseRenamePage
	, private ui::ITextEffectCallback
{
protected:
	CBaseRenameListPage( CRenameFilesDialog* pParentDlg, UINT listLayoutId );

	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;

	virtual void DoSetupFileListView( void ) = 0;
	virtual ren::TSortingPair GetListSorting( void ) const = 0;
public:
	virtual ~CBaseRenameListPage();
private:
	// IRenamePage interface
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem ) override;
	virtual void InvalidateFiles( void ) override;

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override;

	void SetupFileListView( void );
protected:
	// enum { IDD = IDD_REN_LIST_PAGE };

	CReportListControl m_fileListCtrl;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX ) override;
protected:
	afx_msg void OnLvnListSorted_RenameList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnItemChanged_RenameList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


class CRenameSimpleListPage : public CBaseRenameListPage
{
public:
	CRenameSimpleListPage( CRenameFilesDialog* pParentDlg );
protected:
	virtual void DoSetupFileListView( void ) override;
	virtual ren::TSortingPair GetListSorting( void ) const override;
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;
private:
	enum Column { SrcPath, DestPath };

	static std::pair<int, bool> ToListSorting( const ren::TSortingPair& sorting );
	static ren::TSortingPair FromListSorting( const std::pair<int, bool>& listSorting );
};


class CRenameDetailsListPage : public CBaseRenameListPage
{
public:
	CRenameDetailsListPage( CRenameFilesDialog* pParentDlg );
protected:
	virtual void DoSetupFileListView( void ) override;
	virtual ren::TSortingPair GetListSorting( void ) const override;
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;
};


class CRenameEditPage : public CBaseRenamePage
{
public:
	CRenameEditPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameEditPage();

	void CommitLocalEdits( void );
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;

	// IRenamePage interface
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem ) override;

	void SetupFileEdits( void );

	bool InputDestPaths( void );		// validated results in m_newDestPaths
	bool AnyChanges( void ) const;
	bool SelectItem( const CRenameItem* pRenameItem );
	bool SelectItemLine( int linePos );
	bool SyncSelectItemLine( const CTextEdit& fromEdit, CTextEdit* pToEdit );
private:
	std::vector<fs::CPath> m_newDestPaths;
private:
	// enum { IDD = IDD_REN_EDIT_PAGE };

	CThemeStatic m_srcStatic, m_destStatic;
	CTextEdit m_srcEdit;
	CTextEditor m_destEditor;

	CSyncScrolling m_syncScrolling;
	CTextEdit::TLine m_lastCaretLinePos;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlType );
	afx_msg void OnEnChange_DestPaths( void );
	afx_msg void OnEnKillFocus_DestPaths( void );
	afx_msg void OnEnUserSelChange_SrcPaths( void );
	afx_msg void OnEnUserSelChange_DestPaths( void );

	DECLARE_MESSAGE_MAP()
};


#endif // RenamePages_h
