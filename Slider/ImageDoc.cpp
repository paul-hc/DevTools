
#include "stdafx.h"
#include "ImageDoc.h"
#include "MainFrame.h"
#include "ImageView.h"
#include "Application_fwd.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CImageDoc, CDocumentBase )

CImageDoc::CImageDoc( void )
	: CDocumentBase()
{
}

CImageDoc::~CImageDoc()
{
}

CWicImage* CImageDoc::GetImage( void ) const
{
	if ( !m_imagePathKey.first.IsEmpty() )
		return CWicImageCache::Instance().Acquire( m_imagePathKey ).first;

	return NULL;
}

// this overridden version won't call Serialize(), but loads directly the image file
BOOL CImageDoc::OnOpenDocument( LPCTSTR pFilePath )
{
	DeleteContents();
	SetModifiedFlag();				// dirty while loading

	try
	{
		CWaitCursor wait;
		m_imagePathKey.first.Set( pFilePath );
		if ( NULL == GetImage() )
			AfxThrowFileException( m_imagePathKey.first.FileExist( fs::Read ) ? CFileException::accessDenied : CFileException::fileNotFound );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc, MB_OK | MB_ICONERROR, AFX_IDP_FAILED_TO_OPEN_DOC );
		DeleteContents();			// remove failed contents
		return FALSE;
	}

	SetModifiedFlag( FALSE );		// start off with unmodified
	return TRUE;
}

BOOL CImageDoc::OnSaveDocument( LPCTSTR pFilePath )
{
	CWicImage* pImage = GetImage();
	ASSERT_PTR( pImage );
	ASSERT( pImage->IsValid() );

	try
	{
		CWaitCursor wait;

		if ( pImage->GetOrigin().SaveBitmapToFile( pFilePath ) )
		{
			SetModifiedFlag( FALSE );		// back to unmodified
			return TRUE;
		}
		return FALSE;
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc, MB_OK | MB_ICONERROR, AFX_IDP_FAILED_TO_SAVE_DOC );
		return FALSE;
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CImageDoc, CDocumentBase )
	ON_UPDATE_COMMAND_UI( ID_FILE_SAVE, OnUpdateFileSave )
	ON_UPDATE_COMMAND_UI( ID_FILE_SAVE_AS, OnUpdateFileSaveAs )
END_MESSAGE_MAP()

BOOL CImageDoc::OnNewDocument( void )
{
	ui::ReportError( _T("Cannot create an empty image document!"), MB_ICONWARNING );
	return FALSE;
}

void CImageDoc::OnUpdateFileSave( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();
	pCmdUI->Enable( pImage != NULL && pImage->IsValid() );
}

void CImageDoc::OnUpdateFileSaveAs( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage();
	pCmdUI->Enable( pImage != NULL && pImage->IsValid() );
}
