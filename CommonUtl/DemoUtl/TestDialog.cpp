
#include "stdafx.h"
#include "TestDialog.h"
#include "DemoTemplate.h"
#include "resource.h"
#include "utl/UI/CmdUpdate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTestDialog::CTestDialog( CWnd* pParent )
	: CLayoutDialog( IDD_DEMO_DIALOG, pParent )
	, m_pDemo( new CDemoTemplate( this ) )
{
	m_regSection = _T("TestDialog");
}

CTestDialog::~CTestDialog()
{
}

void CTestDialog::DoDataExchange( CDataExchange* pDX )
{
	m_pDemo->DoDataExchange( pDX );

	__super::DoDataExchange( pDX );
}

void CTestDialog::OnIdleUpdateControls( void )
{
	__super::OnIdleUpdateControls();
	ui::UpdateDlgItemUI( this, IDC_PASTE_FILES_BUTTON );
}

BOOL CTestDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pDemo->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BEGIN_MESSAGE_MAP( CTestDialog, CLayoutDialog )
END_MESSAGE_MAP()
