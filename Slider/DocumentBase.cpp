
#include "stdafx.h"
#include "DocumentBase.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC( CDocumentBase, CDocument )

CDocumentBase::CDocumentBase( void )
	: CDocument()
{
}

CWicImage* CDocumentBase::AcquireImage( const fs::ImagePathKey& imageKey )
{
	if ( !imageKey.first.IsEmpty() )
		return CWicImageCache::Instance().Acquire( imageKey ).first;

	return NULL;
}

CDocumentBase::~CDocumentBase()
{
}


// commands

BEGIN_MESSAGE_MAP( CDocumentBase, CDocument )
END_MESSAGE_MAP()
