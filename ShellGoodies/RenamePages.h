#ifndef RenamePages_h
#define RenamePages_h
#pragma once

#include "utl/ISubject.h"
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/SyncScrolling.h"
#include "utl/UI/TextEditor.h"
#include "utl/UI/ThemeStatic.h"


class CFileModel;
class CRenameFilesDialog;
class CRenameItem;


interface IRenamePage : public utl::IObserver
{
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem ) = 0;
	virtual void InvalidateFiles( void ) {}
};


abstract class CBaseRenamePage : public CLayoutPropertyPage
							   , public IRenamePage
{
protected:
	CBaseRenamePage( UINT templateId, CRenameFilesDialog* pParentDlg );
protected:
	CRenameFilesDialog* m_pParentDlg;
};


class CRenameListPage : public CBaseRenamePage
					  , private ui::ITextEffectCallback
{
public:
	CRenameListPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameListPage();
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// IRenamePage interface
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem );
	virtual void InvalidateFiles( void );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;

	void SetupFileListView( void );
private:
	// enum { IDD = IDD_REN_LIST_PAGE };

	CReportListControl m_fileListCtrl;

	enum Column { Source, Destination };

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


class CRenameEditPage : public CBaseRenamePage
					  , private CInternalChange
{
public:
	CRenameEditPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameEditPage();

	void CommitLocalEdits( void );
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// IRenamePage interface
	virtual void EnsureVisibleItem( const CRenameItem* pRenameItem );

	void SetupFileEdits( void );

	bool InputDestPaths( void );		// validated results in m_newDestPaths
	bool AnyChanges( void ) const;
private:
	std::vector< fs::CPath > m_newDestPaths;
private:
	// enum { IDD = IDD_REN_EDIT_PAGE };

	CThemeStatic m_srcStatic, m_destStatic;
	CTextEdit m_srcEdit;
	CTextEditor m_destEditor;

	CSyncScrolling m_syncScrolling;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnEnChange_DestPaths( void );
	afx_msg void OnEnKillFocus_DestPaths( void );

	DECLARE_MESSAGE_MAP()
};


#endif // RenamePages_h
