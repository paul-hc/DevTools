// InputTypeQualifierDialog.cpp : implementation file
//
#include "stdafx.h"
#include "InputTypeQualifierDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


InputTypeQualifierDialog::InputTypeQualifierDialog( const CString& typeQualifier, CWnd* pParent )
	: CDialog( IDD_INPUT_TYPE_QUALIFIER_DIALOG, pParent )
{
	//{{AFX_DATA_INIT(InputTypeQualifierDialog)
	m_typeQualifier = typeQualifier;
	//}}AFX_DATA_INIT
}

void InputTypeQualifierDialog::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
	//{{AFX_DATA_MAP(InputTypeQualifierDialog)
	DDX_Text(pDX, IDC_TYPE_QUALIFIER_EDIT, m_typeQualifier);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(InputTypeQualifierDialog, CDialog)
	//{{AFX_MSG_MAP(InputTypeQualifierDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/**
	InputTypeQualifierDialog message handlers
*/

void InputTypeQualifierDialog::OnOK() 
{
	CDialog::OnOK();

	if ( m_typeQualifier.GetLength() > 2 )
		if ( m_typeQualifier.Right( 2 ) != _T("::") )
			m_typeQualifier += _T("::");
}
