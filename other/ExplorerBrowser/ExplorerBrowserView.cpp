// ExplorerBrowserView.cpp : implementation of the CExplorerBrowserView class
//

#include "stdafx.h"
#include "ExplorerBrowser.h"

#include "ExplorerBrowserDoc.h"
#include "ExplorerBrowserView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CExplorerBrowserView

IMPLEMENT_DYNCREATE(CExplorerBrowserView, CView)

BEGIN_MESSAGE_MAP(CExplorerBrowserView, CView)
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
    ON_COMMAND(ID_VIEW_GOTOUSERPROFILE, OnBrowseToProfileFolder)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND_EX(ID_VIEW_DETAILS, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_LARGEICON, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_LIST, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_SMALLICON, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_THUMBNAILS, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_THUMBSTRIP, &CExplorerBrowserView::OnViewCommand)
    ON_COMMAND_EX(ID_VIEW_TILES, &CExplorerBrowserView::OnViewCommand)
    
    ON_WM_DESTROY()
    ON_COMMAND(ID_VIEW_BACK, &CExplorerBrowserView::OnViewBack)
    ON_COMMAND(ID_VIEW_FORWARD, &CExplorerBrowserView::OnViewForward)
    ON_COMMAND(ID_VIEW_FRAMES, &CExplorerBrowserView::OnViewFrames)
    ON_COMMAND(ID_VIEW_SHOWSELECTION, &CExplorerBrowserView::OnViewShowselection)
END_MESSAGE_MAP()

// CExplorerBrowserView construction/destruction

CExplorerBrowserView::CExplorerBrowserView()
{
    m_showFrames = true;
    m_dwAdviseCookie = 0;

}

CExplorerBrowserView::~CExplorerBrowserView()
{
}

BOOL CExplorerBrowserView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CView::PreCreateWindow(cs))
        return FALSE;

    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    return TRUE;
}

// CExplorerBrowserView drawing

void CExplorerBrowserView::OnDraw(CDC* /*pDC*/)
{
    CExplorerBrowserDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
        return;

    // TODO: add draw code for native data here
}


// CExplorerBrowserView printing

BOOL CExplorerBrowserView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void CExplorerBrowserView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}

void CExplorerBrowserView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}


// CExplorerBrowserView diagnostics

#ifdef _DEBUG
void CExplorerBrowserView::AssertValid() const
{
    CView::AssertValid();
}

void CExplorerBrowserView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CExplorerBrowserDoc* CExplorerBrowserView::GetDocument() const // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CExplorerBrowserDoc)));
    return (CExplorerBrowserDoc*)m_pDocument;
}
#endif //_DEBUG


// CExplorerBrowserView message handlers

int CExplorerBrowserView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ExplorerBrowser, NULL, IID_PPV_ARGS(&m_spExplorerBrowser));

    if (SUCCEEDED(hr))
    {
		if ( g_theApp.m_showFrames )
            m_spExplorerBrowser->SetOptions(EBO_SHOWFRAMES);
        
        FOLDERSETTINGS fs;
        fs.fFlags = 0;
        fs.ViewMode = FVM_DETAILS;
        hr = m_spExplorerBrowser->Initialize(m_hWnd, CRect(0, 0, 0, 0), &fs);
    
        if (SUCCEEDED(hr))
        {
            CComObject<CExplorerBrowserEvents>* pExplorerEvents;
            if (SUCCEEDED(CComObject<CExplorerBrowserEvents>::CreateInstance(&pExplorerEvents)))
            {
                pExplorerEvents->AddRef();
                
                pExplorerEvents->SetView(this);
                m_spExplorerBrowser->Advise(pExplorerEvents, &m_dwAdviseCookie);
                
                pExplorerEvents->Release();
            }
        }
    }

    return SUCCEEDED(hr) ? 0 : -1;
}

void CExplorerBrowserView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    m_spExplorerBrowser->SetRect(NULL, CRect(0, 0, cx, cy));
}

bool CExplorerBrowserView::NavigateTo(LPCTSTR szPath)
{
    HRESULT hr = S_OK;
	LPITEMIDLIST pidlBrowse;

	if (FAILED(hr = SHParseDisplayName(szPath, NULL, &pidlBrowse, 0, NULL)))
    {
        ATLTRACE("SHParseDisplayName Failed! hr = %d\n", hr);
		return false;
	}

    if (FAILED(hr = m_spExplorerBrowser->BrowseToIDList(pidlBrowse, 0)))
        ATLTRACE("BrowseToIDList Failed! hr = %d\n", hr);

    ILFree(pidlBrowse);

	return SUCCEEDED(hr);
}

void CExplorerBrowserView::OnInitialUpdate()
{
    CView::OnInitialUpdate();
    
    HRESULT hr = S_OK;
    //Browse the explorer to the current directory
    
    TCHAR szCurrentDir[_MAX_PATH + 1];
    szCurrentDir[_MAX_PATH] = 0;
    
    LPITEMIDLIST pidlBrowse = NULL;

    if (!GetCurrentDirectory(_MAX_PATH, szCurrentDir) || 
		!NavigateTo(szCurrentDir))
	{
        OnBrowseToProfileFolder();
    }
}

void CExplorerBrowserView::OnBrowseToProfileFolder()
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlBrowse = NULL;

    if (SUCCEEDED(hr = SHGetFolderLocation(NULL, CSIDL_PROFILE, NULL, 0, &pidlBrowse)))
    {
        if (FAILED(hr = m_spExplorerBrowser->BrowseToIDList(pidlBrowse, 0)))
            ATLTRACE("BrowseToIDList Failed! hr = %d\n", hr);

        ILFree(pidlBrowse);
    }
    else
    {
        ATLTRACE("SHGetFolderLocation Failed! hr = %d\n", hr);
    }

    if (FAILED(hr))
        AfxMessageBox(IDP_NAVIGATION_FAILED);
}

BOOL CExplorerBrowserView::OnViewCommand(UINT nID)
{
    UINT uViewMode;

    switch(nID)
    {
    case ID_VIEW_DETAILS:
        uViewMode = FVM_DETAILS;
        break;
    case ID_VIEW_LARGEICON:
        uViewMode = FVM_ICON;
        break;
    case ID_VIEW_LIST:
        uViewMode = FVM_LIST;
        break;
    case ID_VIEW_SMALLICON:
        uViewMode = FVM_SMALLICON;
        break;
    case ID_VIEW_THUMBNAILS:
        uViewMode = FVM_THUMBNAIL;
        break;
    case ID_VIEW_THUMBSTRIP:
        uViewMode = FVM_THUMBSTRIP;
        break;
    case ID_VIEW_TILES:
        uViewMode = FVM_TILE;
        break;
    }
    
    FOLDERSETTINGS fs = {uViewMode, 0};
    m_spExplorerBrowser->SetFolderSettings(&fs);
    return TRUE;
}

void CExplorerBrowserView::OnDestroy()
{
    CView::OnDestroy();
    
    if(m_dwAdviseCookie)
    {
        m_spExplorerBrowser->Unadvise(m_dwAdviseCookie);
    }

    m_spExplorerBrowser->Destroy();
}

void CExplorerBrowserView::OnViewBack()
{
    m_spExplorerBrowser->BrowseToIDList(NULL, SBSP_NAVIGATEBACK);
}

void CExplorerBrowserView::OnViewForward()
{
    m_spExplorerBrowser->BrowseToIDList(NULL, SBSP_NAVIGATEFORWARD);
}

void CExplorerBrowserView::OnViewFrames()
{
    m_showFrames = !m_showFrames;
	m_spExplorerBrowser->SetOptions( m_showFrames ? EBO_SHOWFRAMES : EBO_NONE );
}

static CString GetFolderDisplayName(PCIDLIST_ABSOLUTE pidlFolder)
{
    CString strDisplayName;

    CComPtr<IShellFolder> spSFDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&spSFDesktop)))
    {
        STRRET srDisplayName;
        if (SUCCEEDED(spSFDesktop->GetDisplayNameOf(pidlFolder, SHGDN_FORPARSING, &srDisplayName)))
        {
            LPTSTR szDisplayName = NULL;
            StrRetToStr(&srDisplayName, pidlFolder, &szDisplayName);
            strDisplayName = szDisplayName;
            CoTaskMemFree(szDisplayName);
        }
    }
    
    return strDisplayName;
}

HRESULT CExplorerBrowserView::OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder)
{
    ATLTRACE("OnNavigationPending %S\n", static_cast<LPCTSTR>(GetFolderDisplayName(pidlFolder)));
    return S_OK;
}

void CExplorerBrowserView::OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder)
{
    ATLTRACE("OnNavigationComplete %S\n", static_cast<LPCTSTR>(GetFolderDisplayName(pidlFolder)));
}

void CExplorerBrowserView::OnViewCreated(IShellView *psv)
{
    ATLTRACE("OnViewCreated\n");
}

void CExplorerBrowserView::OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder)
{
    ATLTRACE("OnNavigationFailed %S\n", static_cast<LPCTSTR>(GetFolderDisplayName(pidlFolder)));
}

void CExplorerBrowserView::GetSelectedFiles(CStringArray& arrSelection)
{
    CComPtr<IShellView> spSV;
    if (SUCCEEDED(m_spExplorerBrowser->GetCurrentView(IID_PPV_ARGS(&spSV))))
    {
        CComPtr<IDataObject> spDataObject;
        if (SUCCEEDED(spSV->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&spDataObject))))
        {
			//Code adapted from http://www.codeproject.com/shell/shellextguide1.asp
			FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT,
							  -1, TYMED_HGLOBAL };
			STGMEDIUM stg;
			stg.tymed =  TYMED_HGLOBAL;

			if (SUCCEEDED(spDataObject->GetData(&fmt, &stg)))
			{
				HDROP hDrop = (HDROP) GlobalLock ( stg.hGlobal );

				UINT uNumFiles = DragQueryFile ( hDrop, 0xFFFFFFFF, NULL, 0 );
				HRESULT hr = S_OK;

				for(UINT i = 0; i < uNumFiles; i++)
				{
					TCHAR szPath[_MAX_PATH];
					szPath[0] = 0;
					DragQueryFile(hDrop, i, szPath, MAX_PATH);
			        
					if (szPath[0] != 0)
						arrSelection.Add(szPath);	
				}
			    
				GlobalUnlock ( stg.hGlobal );
				ReleaseStgMedium ( &stg );
            }
        }
    }
    
}

void CExplorerBrowserView::OnViewShowselection()
{
    CStringArray arrSelectedFiles;
    
    GetSelectedFiles(arrSelectedFiles);

    CString strSelectedFileMessage;
    strSelectedFileMessage.Format(IDS_SELECTEDFILES, arrSelectedFiles.GetCount());

    for(INT_PTR i = 0; i < arrSelectedFiles.GetCount(); i++)
    {
        strSelectedFileMessage.Append(arrSelectedFiles[i]);
        strSelectedFileMessage.AppendChar('\n');
    }

    AfxMessageBox(strSelectedFileMessage);
}
