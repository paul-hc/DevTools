
#include "stdafx.h"
#include "BrowserDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CBrowserDoc, CDocument )

CBrowserDoc::CBrowserDoc( void )
{
}

CBrowserDoc::~CBrowserDoc()
{
}

BOOL CBrowserDoc::OnNewDocument( void )
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	return TRUE;
}

void CBrowserDoc::Serialize( CArchive& ar )
{
	if ( ar.IsStoring() )
	{
	}
	else
	{
	}
}


// command handlers

BEGIN_MESSAGE_MAP( CBrowserDoc, CDocument )
END_MESSAGE_MAP()
