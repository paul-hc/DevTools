
#include "stdafx.h"
#include "DefinePasswordDialog.h"
#include "resource.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDefinePasswordDialog::CDefinePasswordDialog( const TCHAR* pStgDocPath, CWnd* pParent /*=NULL*/ )
	: CDialog( IDD_DEFINE_PASSWORD_DIALOG, pParent )
	, m_protectedFileLabel( str::Format( _T("Protect file: %s"), pStgDocPath ) )
{
}

bool CDefinePasswordDialog::Run( void )
{
	for ( ;; )
		if ( DoModal() != IDOK )
			return false;
		else if ( m_password != m_confirmPassword )
		{
			m_password.clear();
			m_confirmPassword.clear();

			AfxMessageBox( _T("Password mismatch!\nPlease enter the password again."), MB_ICONERROR | MB_OK );
		}
		else
			break;

	return true;
}

void CDefinePasswordDialog::DoDataExchange( CDataExchange* pDX )
{
	ui::DDX_Text( pDX, IDC_PASSWORD_EDIT, m_password );
	ui::DDX_Text( pDX, IDC_CONFIRM_PASSWORD_EDIT, m_confirmPassword );
	ui::DDX_Text( pDX, IDC_FILE_PROTECTED_STATIC, m_protectedFileLabel );

	CDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CDefinePasswordDialog, CDialog )
END_MESSAGE_MAP()
