
#include "StdAfx.h"
#include "InputPasswordDialog.h"
#include "resource.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CInputPasswordDialog::CInputPasswordDialog( const TCHAR* pCompoundFilePath, CWnd* pParent /*=NULL*/ )
	: CDialog( IDD_INPUT_PASSWORD_DIALOG, pParent )
	, m_protectedFileLabel( str::Format( _T("'%s' is protected."), pCompoundFilePath ) )
{
}

void CInputPasswordDialog::DoDataExchange( CDataExchange* pDX )
{
	ui::DDX_Text( pDX, IDC_PASSWORD_EDIT, m_password );
	ui::DDX_Text( pDX, IDC_FILE_PROTECTED_STATIC, m_protectedFileLabel );

	CDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CInputPasswordDialog, CDialog )
END_MESSAGE_MAP()
