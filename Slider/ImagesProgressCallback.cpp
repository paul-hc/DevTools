
#include "stdafx.h"
#include "ImagesProgressCallback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const std::tstring CImagesProgressCallback::s_searching = _T("Searching for Image Files");

CImagesProgressCallback::CImagesProgressCallback( CWnd* pParentWnd, const std::tstring& operationLabel /*= s_searching*/ )
	: m_dlg( operationLabel, CProgressDialog::StageLabelCount )
{
	if ( m_dlg.Create( _T("Image Files"), pParentWnd ) )
	{
		m_dlg.SetStageLabel( _T("Search directory") );
		m_dlg.SetItemLabel( _T("Found image") );
		m_dlg.SetMarqueeProgress();
	}
}

CImagesProgressCallback::~CImagesProgressCallback()
{
	m_dlg.DestroyWindow();
}

void CImagesProgressCallback::Section_OrderImageFiles( size_t fileCount )
{
	m_dlg.SetOperationLabel( _T("Order Images") );
	m_dlg.ShowStage( false );
	m_dlg.SetItemLabel( _T("Compare image file attributes") );

	GetCallback()->SetProgressItemCount( fileCount );
}

void CImagesProgressCallback::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	GetCallback()->AdvanceItem( pFilePath );
}

bool CImagesProgressCallback::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	GetCallback()->AdvanceStage( pSubDirPath );
	return true;
}
