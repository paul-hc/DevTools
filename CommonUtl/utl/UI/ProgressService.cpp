
#include "stdafx.h"
#include "ProgressService.h"
#include "ProgressDialog.h"
#include "utl/AppTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


std::tstring CProgressService::s_dialogTitle = _T("Files");
std::tstring CProgressService::s_stageLabel = _T("Search directory");
std::tstring CProgressService::s_itemLabel = _T("Found file");

CProgressService::CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel )
{
	// prevent displaying the CAlbumModel::SearchForFiles() progress dialog when opening folders at application start-up:
	if ( app::IsInteractive() )
	{
		m_pDlg.reset( new CProgressDialog( operationLabel, CProgressDialog::StageLabelCount ) );

		if ( m_pDlg->Create( s_dialogTitle, pParentWnd != NULL ? pParentWnd : AfxGetMainWnd() ) )
		{
			GetHeader()->SetStageLabel( s_stageLabel );
			GetHeader()->SetItemLabel( s_itemLabel );

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
	if ( m_pDlg.get() != NULL )
		m_pDlg->DestroyWindow();
}

utl::IProgressService* CProgressService::GetService( void )
{
	if ( !app::IsInteractive() )
		return svc::CNoProgressService::Instance();

	return m_pDlg->GetService();
}

void CProgressService::OnAddFileInfo( const CFileFind& foundFile )
{
	foundFile;
	ASSERT( false );		// normally should not be chained for progess; AddFoundFile() should be called instead
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