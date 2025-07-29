// Host Windows Explorer in your applications using the new Vista hosting APIs:
// Rama Krishna Vavilala, Feb 2007:
//	https://www.codeproject.com/Articles/17809/Host-Windows-Explorer-in-your-applications-using-t

#pragma once

#include "ExplorerBrowser.h"


class CBrowserDoc;


class CBrowserView : public CView
{
	DECLARE_DYNCREATE( CBrowserView )
protected:
	CBrowserView( void );
	virtual ~CBrowserView();
public:
	CBrowserDoc* GetDocument( void ) const { return reinterpret_cast<CBrowserDoc*>( m_pDocument ); }

	// event callbacks
	HRESULT OnNavigationPending( PCIDLIST_ABSOLUTE pidlFolder );
	void OnNavigationComplete( PCIDLIST_ABSOLUTE pidlFolder );
	void OnViewCreated( IShellView* pShellView );
	void OnNavigationFailed( PCIDLIST_ABSOLUTE pidlFolder );
private:
	static FOLDERVIEWMODE CmdToViewMode( UINT cmdId );
private:
	std::auto_ptr<shell::CExplorerBrowser> m_pBrowser;
	bool m_showFrames;
	DWORD m_dwAdviseCookie;
protected:

	// generated stuff
public:
	virtual void OnInitialUpdate( void );
	virtual void OnDraw( CDC* pDC );
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnFileRename( void );
	afx_msg void OnViewMode( UINT cmdId );
	afx_msg void OnUpdateViewMode( CCmdUI* pCmdUI );
	afx_msg void OnViewFrames( void );
	afx_msg void OnUpdateViewFrames( CCmdUI* pCmdUI );
	afx_msg void OnViewShowselection( void );
	afx_msg void OnFolderBack( void );
	afx_msg void OnFolderForward( void );
	afx_msg void OnFolderUp( void );
	afx_msg void OnFolderUser( void );

	DECLARE_MESSAGE_MAP()
};


class CExplorerBrowserEvents : public CComObjectRootEx<CComSingleThreadModel>
							 , public IExplorerBrowserEvents
{
public:
	CExplorerBrowserEvents( void ) {}

	void SetView( CBrowserView* pView ) { m_pView = pView; }
private:
	CBrowserView* m_pView;
public:
	BEGIN_COM_MAP( CExplorerBrowserEvents )
		COM_INTERFACE_ENTRY( IExplorerBrowserEvents )
	END_COM_MAP()
public:
	STDMETHOD( OnNavigationPending )( PCIDLIST_ABSOLUTE pidlFolder )
	{
		return m_pView != NULL ? m_pView->OnNavigationPending( pidlFolder ) : E_FAIL;
	}

	STDMETHOD( OnViewCreated )( IShellView* psv )
	{
		if ( m_pView != NULL )
			m_pView->OnViewCreated( psv );
		return S_OK;
	}

	STDMETHOD( OnNavigationComplete )( PCIDLIST_ABSOLUTE pidlFolder )
	{
		if ( m_pView != NULL )
			m_pView->OnNavigationComplete(pidlFolder);
		return S_OK;
	}

	STDMETHOD( OnNavigationFailed )( PCIDLIST_ABSOLUTE pidlFolder )
	{
		if ( m_pView != NULL )
			m_pView->OnNavigationFailed( pidlFolder );
		return S_OK;
	}
};
