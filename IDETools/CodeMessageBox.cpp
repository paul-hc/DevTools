
#include "stdafx.h"
#include "CodeMessageBox.h"
#include "CodeUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("CCodeMessageBox");
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_MESSAGE_STATIC, SizeX },
		{ IDC_CODE_FIELD_EDIT, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CCodeMessageBox::CCodeMessageBox( const CString& message, const CString& codeText, UINT mbType /*= MB_ICONQUESTION*/,
								  const CString& caption /*= AfxGetApp()->m_pszProfileName*/, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_CODE_MESSAGE_BOX_DIALOG, pParent )
	, m_caption( caption )
	, m_message( message )
	, m_codeText( codeText )
	, m_mbType( mbType )
{
	m_initCentered = true;
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_codeText = code::convertTabsToSpaces( m_codeText );
}

CCodeMessageBox::~CCodeMessageBox()
{
}

void CCodeMessageBox::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_ICON_STATIC, m_iconStatic );
	DDX_Control( pDX, IDC_MESSAGE_STATIC, m_messageStatic );
	DDX_Control( pDX, IDC_CODE_FIELD_EDIT, m_codeEdit );

	CLayoutDialog::DoDataExchange( pDX );
}

// message handlers

BEGIN_MESSAGE_MAP( CCodeMessageBox, CLayoutDialog )
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CCodeMessageBox::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();

	HICON hIcon = NULL;

	switch ( m_mbType & 0x00F0 )
	{
		case MB_ICONHAND:			hIcon = ::LoadIcon( NULL, IDI_HAND ); break;
		case MB_ICONQUESTION:		hIcon = ::LoadIcon( NULL, IDI_QUESTION ); break;
		case MB_ICONEXCLAMATION:	hIcon = ::LoadIcon( NULL, IDI_EXCLAMATION ); break;
		case MB_ICONASTERISK:		hIcon = ::LoadIcon( NULL, IDI_INFORMATION ); break;
	}
	if ( hIcon != NULL )
		m_iconStatic.SetIcon( hIcon );

	if ( CFont* pSrcFont = GetFont() )
	{
		LOGFONT boldLogFont;
		pSrcFont->GetLogFont( &boldLogFont );
		boldLogFont.lfWeight = FW_BOLD;
		if ( messageFont.CreateFontIndirect( &boldLogFont ) )
			m_messageStatic.SetFont( &messageFont );
	}
	m_messageStatic.SetWindowText( m_message );

	m_codeEdit.SetWindowText( m_codeText );
	m_codeEdit.SetSel( 0, 0 );

	SetWindowText( m_caption );
	return TRUE;
}

HBRUSH CCodeMessageBox::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType )
{
	HBRUSH hBrushBkgnd = CLayoutDialog::OnCtlColor( pDC, pWnd, ctlColorType );

	if ( pWnd == &m_codeEdit )
		pDC->SetTextColor( 0xFF0000 );
	return hBrushBkgnd;
}
