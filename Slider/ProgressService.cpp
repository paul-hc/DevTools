
#include "stdafx.h"
#include "ProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const std::tstring CProgressService::s_searching = _T("Searching for Image Files");

CProgressService::CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel /*= s_searching*/ )
	: m_dlg( operationLabel, CProgressDialog::StageLabelCount )
{
	if ( m_dlg.Create( _T("Image Files"), pParentWnd != NULL ? pParentWnd : AfxGetMainWnd() ) )
	{
		GetHeader()->SetStageLabel( _T("Search directory") );
		GetHeader()->SetItemLabel( _T("Found image") );

		GetService()->SetMarqueeProgress();
	}
}

CProgressService::~CProgressService()
{
	DestroyDialog();
}

void CProgressService::DestroyDialog( void )
{
	m_dlg.DestroyWindow();
}

void CProgressService::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	GetService()->AdvanceItem( pFilePath );
}

bool CProgressService::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	GetService()->AdvanceStage( pSubDirPath );
	return true;
}
