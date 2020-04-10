
#include "stdafx.h"
#include "SearchImagesProgressCallback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSearchImagesProgressCallback::CSearchImagesProgressCallback( CWnd* pParent )
	: m_dlg( _T("Searching for Image Files"), CProgressDialog::StageLabelCount )
{
	if ( m_dlg.Create( _T("Image Files Search"), pParent ) )
	{
		m_dlg.SetStageLabel( _T("Search directory") );
		m_dlg.SetItemLabel( _T("Found image") );
		m_dlg.SetMarqueeProgress();
	}
}

CSearchImagesProgressCallback::~CSearchImagesProgressCallback()
{
	m_dlg.DestroyWindow();
}

void CSearchImagesProgressCallback::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	m_dlg.AdvanceItem( pFilePath );
}

bool CSearchImagesProgressCallback::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	m_dlg.AdvanceStage( pSubDirPath );
	return true;
}
