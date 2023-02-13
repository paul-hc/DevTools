
#include "pch.h"
#include "InputTypeQualifierDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CInputTypeQualifierDialog::CInputTypeQualifierDialog( const CString& typeQualifier, CWnd* pParent )
	: CDialog( IDD_INPUT_TYPE_QUALIFIER_DIALOG, pParent )
{
	m_typeQualifier = typeQualifier;
}

void CInputTypeQualifierDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Text( pDX, IDC_TYPE_QUALIFIER_EDIT, m_typeQualifier );

	CDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CInputTypeQualifierDialog, CDialog )
END_MESSAGE_MAP()

void CInputTypeQualifierDialog::OnOK() 
{
	CDialog::OnOK();

	if ( m_typeQualifier.GetLength() > 2 )
		if ( m_typeQualifier.Right( 2 ) != _T("::") )
			m_typeQualifier += _T("::");
}
