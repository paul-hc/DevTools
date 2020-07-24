
#include "stdafx.h"
#include "ProgressService.h"
#include "Application_fwd.h"
#include "utl/UI/ProgressDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const std::tstring CProgressService::s_searching = _T("Searching for Image Files");

CProgressService::CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel /*= s_searching*/ )
{
	// prevent displaying the CAlbumModel::SearchForFiles() progress dialog when opening folders at application start-up:
	if ( app::IsInteractive() )
	{
		m_pDlg.reset( new CProgressDialog( operationLabel, CProgressDialog::StageLabelCount ) );

		if ( m_pDlg->Create( _T("Image Files"), pParentWnd != NULL ? pParentWnd : AfxGetMainWnd() ) )
		{
			GetHeader()->SetStageLabel( _T("Search directory") );
			GetHeader()->SetItemLabel( _T("Found image") );

			GetService()->SetMarqueeProgress();
		}
	}
}

CProgressService::CProgressService( void )
{
}

CProgressService::~CProgressService()
{
	DestroyDialog();
}

void CProgressService::DestroyDialog( void )
{
	if ( IsInteractive() )
		m_pDlg->DestroyWindow();
}

ui::IProgressService* CProgressService::GetService( void )
{
	if ( !IsInteractive() )
		return ui::CNoProgressService::Instance();

	return m_pDlg->GetService();
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
