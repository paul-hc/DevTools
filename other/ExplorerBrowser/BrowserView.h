#pragma once


namespace shell
{
	bool NavigateTo( const TCHAR path[], IExplorerBrowser* pExplorerBrowser );
	void QuerySelectedFiles( std::vector< std::tstring >& rSelPaths, IExplorerBrowser* pExplorerBrowser );
	FOLDERVIEWMODE GetFolderViewMode( IExplorerBrowser* pExplorerBrowser, UINT* pFolderFlags = NULL );
	std::tstring GetFolderDisplayName( PCIDLIST_ABSOLUTE pidlFolder );
}


class CBrowserDoc;


class CBrowserView : public CView
{
	DECLARE_DYNCREATE( CBrowserView )
protected: // create from serialization only
	CBrowserView();
	virtual ~CBrowserView();
public:
	CBrowserDoc* GetDocument( void ) const { return reinterpret_cast< CBrowserDoc* >( m_pDocument ); }
private:
	// event callbacks
	HRESULT OnNavigationPending( PCIDLIST_ABSOLUTE pidlFolder );
	void OnNavigationComplete( PCIDLIST_ABSOLUTE pidlFolder );
	void OnViewCreated( IShellView* psv );
	void OnNavigationFailed( PCIDLIST_ABSOLUTE pidlFolder );

	friend class CExplorerBrowserEvents;
private:
	CComPtr< IExplorerBrowser > m_pExplorerBrowser;
	bool m_showFrames;
	DWORD m_dwAdviseCookie;
protected:
	// generated stuff
	public:
	virtual void OnInitialUpdate( void );
	virtual void OnDraw( CDC* pDC );
	protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
	virtual BOOL OnPreparePrinting( CPrintInfo* pInfo );
	virtual void OnBeginPrinting( CDC* pDC, CPrintInfo* pInfo );
	virtual void OnEndPrinting( CDC* pDC, CPrintInfo* pInfo );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnBrowseToProfileFolder();
	afx_msg void OnViewMode( UINT cmdId );
	afx_msg void OnUpdateViewMode( CCmdUI* pCmdUI );
	afx_msg void OnViewBack();
	afx_msg void OnViewForward();
	afx_msg void OnViewFrames();
	afx_msg void OnViewShowselection();

	DECLARE_MESSAGE_MAP()
};


class CExplorerBrowserEvents : public CComObjectRootEx< CComSingleThreadModel >
							 , public IExplorerBrowserEvents
{
public:
	CExplorerBrowserEvents( void )
	{
	}

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
		return m_pView ? m_pView->OnNavigationPending( pidlFolder ) : E_FAIL;
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
