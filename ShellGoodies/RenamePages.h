#ifndef RenamePages_h
#define RenamePages_h
#pragma once

#include "utl/ISubject.h"
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/PathItemListCtrl.h"
#include "utl/UI/SelectionData.h"
#include "utl/UI/SyncScrolling.h"
#include "utl/UI/ThemeStatic.h"
#include "Application_fwd.h"


class CFileModel;
class CRenameFilesDialog;
class CRenameItem;


interface IRenamePage : public utl::IObserver
{
	virtual void InvalidateFiles( void ) {}
	virtual bool OnParentCommand( UINT cmdId ) const = 0;
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
	virtual void DoUpdateFileListViewItems( const std::vector<CRenameItem*>& selItems ) = 0;
	virtual ren::TSortingPair GetListSorting( void ) const = 0;
public:
	virtual ~CBaseRenameListPage();
private:
	// IRenamePage interface
	virtual void InvalidateFiles( void ) override;
	virtual bool OnParentCommand( UINT cmdId ) const implement;

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override;

	void SetupFileListView( void );
	void UpdateFileListViewItems( const std::vector<CRenameItem*>& selItems );
protected:
	// enum { IDD = IDD_REN_LIST_PAGE };

	CPathItemListCtrl m_fileListCtrl;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX ) override;
protected:
	afx_msg void OnLvnListSorted_RenameList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnCopyTableText_RenameList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnItemChanged_RenameList( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


class CRenameSimpleListPage : public CBaseRenameListPage
{
public:
	CRenameSimpleListPage( CRenameFilesDialog* pParentDlg );
protected:
	virtual void DoSetupFileListView( void ) override;
	virtual void DoUpdateFileListViewItems( const std::vector<CRenameItem*>& selItems ) override;
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
	virtual void DoUpdateFileListViewItems( const std::vector<CRenameItem*>& selItems ) override;
	virtual ren::TSortingPair GetListSorting( void ) const override;
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;
};


#include "utl/UI/TextListEditor.h"


class CDestFilenameAdapter;


class CRenameEditPage : public CBaseRenamePage
	, private ui::ITextInput
{
public:
	CRenameEditPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameEditPage();

	void CommitLocalEdits( void );
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override;

	// ui::ITextInput interface
	virtual ui::ITextInput::Result OnEditInput( IN OUT ui::CTextValidator& rValidator ) implement;

	// IRenamePage interface
	virtual bool OnParentCommand( UINT cmdId ) const implement;

	void SetupFileEdits( void );
	void UpdateFileEdits( const std::vector<CRenameItem*>& selRenameItems );

	bool InputDestPaths( OUT std::vector<fs::CPath>& rNewDestPaths );		// validated results
	bool AnyChanges( const std::vector<fs::CPath>& newDestPaths ) const;

	bool SelectItems( const ui::CSelectionData<CRenameItem>& selData );
	bool SyncSelectItemLine( const CTextListEditor& fromEdit, CTextListEditor* pToEdit );
private:
	std::auto_ptr<CDestFilenameAdapter> m_pDestAdapter;
private:
	// enum { IDD = IDD_REN_EDIT_PAGE };

	CThemeStatic m_srcStatic, m_destStatic;
	CTextListEditor m_srcEdit;		// read-only
	CTextListEditor m_destEdit;

	CSyncScrolling m_syncScrolling;
protected:
	// base overrides
	virtual bool RestoreFocusControl( void ) override;

	// generated stuff
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlType );
	afx_msg void OnEnUserSelChange_SrcPaths( void );
	afx_msg void OnEnUserSelChange_DestPaths( void );

	DECLARE_MESSAGE_MAP()
};


#endif // RenamePages_h
