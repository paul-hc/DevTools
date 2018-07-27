#ifndef RenamePages_h
#define RenamePages_h
#pragma once

#include "utl/ISubject.h"
#include "utl/LayoutPropertyPage.h"
#include "utl/ReportListControl.h"
#include "utl/SyncScrolling.h"
#include "utl/TextEditor.h"
#include "utl/ThemeStatic.h"


class CFileModel;
class CRenameFilesDialog;
class CPathItemBase;


interface IRenamePage : public utl::IObserver
{
	virtual void EnsureVisibleItem( const CPathItemBase* pItem ) = 0;
	virtual void InvalidateFiles( void ) = 0;
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
					  , private CReportListControl::ITextEffectCallback
{
public:
	CRenameListPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameListPage();
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// IRenamePage interface
	virtual void EnsureVisibleItem( const CPathItemBase* pItem );
	virtual void InvalidateFiles( void );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;

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
{
public:
	CRenameEditPage( CRenameFilesDialog* pParentDlg );
	virtual ~CRenameEditPage();
private:
	// utl::IObserver interface (via IRenamePage)
	virtual void OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage );

	// IRenamePage interface
	virtual void EnsureVisibleItem( const CPathItemBase* pItem );
	virtual void InvalidateFiles( void );

	void SetupFileEdits( void );
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
	DECLARE_MESSAGE_MAP()
};


#endif // RenamePages_h
