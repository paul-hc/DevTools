// ExplorerBrowserView.h : interface of the CExplorerBrowserView class
//


#pragma once


class CExplorerBrowserView : public CView
{
private:
    CComPtr<IExplorerBrowser> m_spExplorerBrowser;
	bool m_showFrames;
	DWORD m_dwAdviseCookie;

protected: // create from serialization only
	CExplorerBrowserView();
	DECLARE_DYNCREATE(CExplorerBrowserView)

// Attributes
public:
	CExplorerBrowserDoc* GetDocument() const;

// Operations
public:
	void GetSelectedFiles(CStringArray& arrSelection);
	bool NavigateTo(LPCTSTR szPath);

// Overrides
protected:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CExplorerBrowserView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnBrowseToProfileFolder();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnInitialUpdate();
	afx_msg BOOL OnViewCommand(UINT nID);
	afx_msg void OnDestroy();
	afx_msg void OnViewBack();
	afx_msg void OnViewForward();
	afx_msg void OnViewFrames();
	afx_msg void OnViewShowselection();

protected:
    virtual HRESULT OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder);
    virtual void OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder);
    virtual void OnViewCreated(IShellView *psv);
    virtual void OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder);
    
	friend class CExplorerBrowserEvents;
};

#ifndef _DEBUG  // debug version in ExplorerBrowserView.cpp
inline CExplorerBrowserDoc* CExplorerBrowserView::GetDocument() const
   { return reinterpret_cast<CExplorerBrowserDoc*>(m_pDocument); }
#endif

class CExplorerBrowserEvents :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IExplorerBrowserEvents
{
private:
    CExplorerBrowserView* m_pView;

public:
    BEGIN_COM_MAP(CExplorerBrowserEvents)
        COM_INTERFACE_ENTRY(IExplorerBrowserEvents)
    END_COM_MAP()

public:
    void SetView(CExplorerBrowserView* pView)
    {
        m_pView = pView;
    }

public:	
    STDMETHOD(OnNavigationPending)(PCIDLIST_ABSOLUTE pidlFolder)
    {
        return m_pView ? m_pView->OnNavigationPending(pidlFolder) : E_FAIL;
    }

    STDMETHOD(OnViewCreated)(IShellView *psv)
    {
        if (m_pView)
            m_pView->OnViewCreated(psv);
        return S_OK;
    }

    STDMETHOD(OnNavigationComplete)(PCIDLIST_ABSOLUTE pidlFolder)
    {
        if (m_pView)
            m_pView->OnNavigationComplete(pidlFolder);
        return S_OK;
    }

    STDMETHOD(OnNavigationFailed)(PCIDLIST_ABSOLUTE pidlFolder)
    {
        if (m_pView)
            m_pView->OnNavigationFailed(pidlFolder);
        return S_OK;
    }
};
