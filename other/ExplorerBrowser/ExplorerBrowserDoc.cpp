// ExplorerBrowserDoc.cpp : implementation of the CExplorerBrowserDoc class
//

#include "stdafx.h"
#include "ExplorerBrowser.h"

#include "ExplorerBrowserDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CExplorerBrowserDoc

IMPLEMENT_DYNCREATE(CExplorerBrowserDoc, CDocument)

BEGIN_MESSAGE_MAP(CExplorerBrowserDoc, CDocument)
END_MESSAGE_MAP()


// CExplorerBrowserDoc construction/destruction

CExplorerBrowserDoc::CExplorerBrowserDoc()
{
	// TODO: add one-time construction code here

}

CExplorerBrowserDoc::~CExplorerBrowserDoc()
{
}

BOOL CExplorerBrowserDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CExplorerBrowserDoc serialization

void CExplorerBrowserDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CExplorerBrowserDoc diagnostics

#ifdef _DEBUG
void CExplorerBrowserDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CExplorerBrowserDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CExplorerBrowserDoc commands
