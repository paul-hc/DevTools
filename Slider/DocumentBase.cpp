
#include "stdafx.h"
#include "DocumentBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC( CDocumentBase, CDocument )

CDocumentBase::CDocumentBase( void )
	: CDocument()
{
}

CDocumentBase::~CDocumentBase()
{
}


// commands

BEGIN_MESSAGE_MAP( CDocumentBase, CDocument )
END_MESSAGE_MAP()
