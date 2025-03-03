
#include "pch.h"
#include "CodeMessageBox.h"
#include "utl/CodeAlgorithms.h"
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


CCodeMessageBox::CCodeMessageBox( const std::tstring& message, const std::tstring& codeText, UINT mbType /*= MB_ICONQUESTION*/, CWnd* pParent /*= nullptr*/ )
	: CLayoutDialog( IDD_CODE_MESSAGE_BOX_DIALOG, pParent )
	, m_caption( AfxGetApp()->m_pszProfileName )
	, m_message( message )
	, m_codeText( codeText )
	, m_mbType( mbType )
{
	m_initCentered = true;
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	m_codeText = code::Untabify( m_codeText );
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

	HICON hIcon = nullptr;

	switch ( m_mbType & 0x00F0 )
	{
		case MB_ICONHAND:			hIcon = ::LoadIcon( nullptr, IDI_HAND ); break;
		case MB_ICONQUESTION:		hIcon = ::LoadIcon( nullptr, IDI_QUESTION ); break;
		case MB_ICONEXCLAMATION:	hIcon = ::LoadIcon( nullptr, IDI_EXCLAMATION ); break;
		case MB_ICONASTERISK:		hIcon = ::LoadIcon( nullptr, IDI_INFORMATION ); break;
	}
	if ( hIcon != nullptr )
		m_iconStatic.SetIcon( hIcon );

	if ( CFont* pSrcFont = GetFont() )
	{
		LOGFONT boldLogFont;
		pSrcFont->GetLogFont( &boldLogFont );
		boldLogFont.lfWeight = FW_BOLD;
		if ( messageFont.CreateFontIndirect( &boldLogFont ) )
			m_messageStatic.SetFont( &messageFont );
	}
	ui::SetWindowText( m_messageStatic, m_message );

	ui::SetWindowText( m_codeEdit, m_codeText );
	m_codeEdit.SetSel( 0, 0 );

	ui::SetWindowText( m_hWnd, m_caption );
	return TRUE;
}

HBRUSH CCodeMessageBox::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType )
{
	HBRUSH hBrushBkgnd = CLayoutDialog::OnCtlColor( pDC, pWnd, ctlColorType );

	if ( pWnd == &m_codeEdit )
		pDC->SetTextColor( 0xFF0000 );
	return hBrushBkgnd;
}
