
#include "stdafx.h"
#include "InputBoxDialog.h"
#include "resource.h"

#ifdef	_DEBUG
#define new	DEBUG_NEW
#endif


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_INPUT_EDIT, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CInputBoxDialog::CInputBoxDialog( const TCHAR* pTitle /*= NULL*/, const TCHAR* pPrompt /*= NULL*/,
								const TCHAR* pInputText /*= NULL*/, CWnd* parent /*= NULL*/ )
	: CLayoutDialog( IDD_INPUT_BOX_DIALOG, parent )
	, m_pTitle( pTitle )
	, m_pPrompt( pPrompt )
	, m_inputText( pInputText )
{
	m_regSection = _T("InputBoxDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
}

CInputBoxDialog::~CInputBoxDialog()
{
}

void CInputBoxDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == inputEdit.m_hWnd;

	DDX_Control( pDX, IDC_INPUT_EDIT, inputEdit );
	DDX_Text( pDX, IDC_INPUT_EDIT, m_inputText );

	if ( firstInit )
	{
		if ( !str::IsEmpty( m_pTitle ) )
			SetWindowText( m_pTitle );

		if ( !str::IsEmpty( m_pPrompt ) )
			SetDlgItemText( IDC_INPUT_STRING_LABEL, m_pPrompt );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CInputBoxDialog, CLayoutDialog )
END_MESSAGE_MAP()
