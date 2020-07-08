
#include "stdafx.h"
#include "ImageDoc.h"
#include "ImageView.h"
#include "MainFrame.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImage.h"

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

CWicImage* CImageDoc::GetImage( UINT framePos ) const
{
	return AcquireImage( fs::ImagePathKey( m_imagePath, framePos ) );
}

CWicImage* CImageDoc::GetCurrentImage( void ) const
{
	return GetImage( 0 );
}

bool CImageDoc::QuerySelectedImagePaths( std::vector< fs::CFlexPath >& rSelImagePaths ) const
{
	rSelImagePaths.push_back( m_imagePath );
	return true;
}


// this overridden version won't call Serialize(), but loads directly the image file
BOOL CImageDoc::OnOpenDocument( LPCTSTR pFilePath )
{
	DeleteContents();
	SetModifiedFlag();				// dirty while loading

	try
	{
		CWaitCursor wait;
		m_imagePath.Set( pFilePath );

		if ( m_imagePath.IsComplexPath() )
		{
			m_storageHost.Clear();
			m_storageHost.Push( m_imagePath.GetPhysicalPath() );
		}

		if ( NULL == GetImage( 0 ) )
			AfxThrowFileException( m_imagePath.FileExist( fs::Read ) ? CFileException::accessDenied : CFileException::fileNotFound, -1, pFilePath );
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
	CWicImage* pImage = GetImage( 0 );
	ASSERT_PTR( pImage );
	ASSERT( pImage->IsValid() );

	try
	{
		CWaitCursor wait;

		if ( !m_imagePath.IsComplexPath() )
		{
			if ( !pImage->GetOrigin().SaveBitmapToFile( pFilePath ) )
				return FALSE;
		}
		else
		{
			// TODO: SaveAs embedded image as normal image file (Save is disabled)
		}
		SetModifiedFlag( FALSE );		// back to unmodified
		return TRUE;
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

	ON_COMMAND( ID_IMAGE_DELETE, On_ImageDelete )
	ON_UPDATE_COMMAND_UI( ID_IMAGE_DELETE, OnUpdate_ImagePhysicalFileWriteOp )
	ON_COMMAND( ID_IMAGE_MOVE, On_ImageMove )
	ON_UPDATE_COMMAND_UI( ID_IMAGE_MOVE, OnUpdate_ImagePhysicalFileWriteOp )
END_MESSAGE_MAP()

BOOL CImageDoc::OnNewDocument( void )
{
	ASSERT( false );		// cannot create an empty image document
	return FALSE;
}

void CImageDoc::OnUpdateFileSave( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage( 0 );
	pCmdUI->Enable( pImage != NULL && pImage->IsValid() && !m_imagePath.IsComplexPath() );
}

void CImageDoc::OnUpdateFileSaveAs( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetImage( 0 );
	pCmdUI->Enable( pImage != NULL && pImage->IsValid() );
}


void CImageDoc::On_ImageDelete( void )
{
	std::vector< fs::CFlexPath > complexPaths;
	std::vector< fs::CPath > physicalPaths;

	if ( QuerySelectedImagePaths( complexPaths ) )
		if ( path::ExtractPhysicalPaths( physicalPaths, complexPaths ) )
			shell::DeleteFiles( physicalPaths );
}

void CImageDoc::On_ImageMove( void )
{
	std::vector< fs::CFlexPath > complexPaths;
	std::vector< fs::CPath > physicalPaths;

	if ( QuerySelectedImagePaths( complexPaths ) )
		if ( path::ExtractPhysicalPaths( physicalPaths, complexPaths ) )
			app::MoveFiles( physicalPaths );
}

void CImageDoc::OnUpdate_ImagePhysicalFileWriteOp( CCmdUI* pCmdUI )
{
	CWicImage* pImage = GetCurrentImage();
	pCmdUI->Enable( pImage != NULL && pImage->IsValidPhysicalFile( fs::Write ) );
}
