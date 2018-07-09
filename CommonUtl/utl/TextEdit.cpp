
#include "stdafx.h"
#include "TextEdit.h"
#include "Dialog_fwd.h"
#include "Icon.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTextEdit implementation

static ACCEL editKeys[] =
{
	{ FVIRTKEY | FCONTROL, _T('A'), ID_EDIT_SELECT_ALL }
};

CTextEdit::CTextEdit( bool useFixedFont /*= true*/ )
	: CBaseFrameHostCtrl< CEdit >()
	, m_useFixedFont( useFixedFont )
	, m_keepSelOnFocus( false )
	, m_hookThumbTrack( true )
	, m_visibleWhiteSpace( false )
	, m_accel( editKeys, COUNT_OF( editKeys ) )
{
}

CTextEdit::~CTextEdit()
{
}

bool CTextEdit::HasInvalidText( void ) const
{
	return false;
}

bool CTextEdit::NormalizeText( void )
{
	return false;
}

enum Whitespace { Tab, LineEnd };
static const struct { LPCWSTR m_pText, m_pEdit; } s_whitespaces[] =
{
	{ L"\t", L"\x2192\t" },
	{ L"\r\n", L"¤\r\n" }
};

std::tstring CTextEdit::GetText( void ) const
{
	std::tstring text;
	ui::GetWindowText( text, m_hWnd );		// avoid an extra copy

	if ( m_visibleWhiteSpace )
		for ( int i = 0; i != COUNT_OF( s_whitespaces ); ++i )
			str::Replace( text, s_whitespaces[ i ].m_pEdit, s_whitespaces[ i ].m_pText );

	return text;
}

bool CTextEdit::SetText( const std::tstring& text )
{
	CScopedInternalChange internalChange( this );
	SetModify( FALSE );

	if ( m_visibleWhiteSpace )
	{
		std::tstring editText = text;
		for ( int i = 0; i != COUNT_OF( s_whitespaces ); ++i )
			str::Replace( editText, s_whitespaces[ i ].m_pText, s_whitespaces[ i ].m_pEdit );

		return ui::SetWindowText( m_hWnd, editText );
	}

	return ui::SetWindowText( m_hWnd, text );
}

void CTextEdit::DDX_Text( CDataExchange* pDX, std::tstring& rValue, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( rValue );
	else
		rValue = GetText();
}

void CTextEdit::DDX_UiEscapeSeqs( CDataExchange* pDX, std::tstring& rValue, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( ui::FormatEscapeSeq( rValue ) );
	else
		rValue = ui::ParseEscapeSeqs( GetText() );
}

bool CTextEdit::SetVisibleWhiteSpace( bool visibleWhiteSpace /*= true*/ )
{
	if ( visibleWhiteSpace == m_visibleWhiteSpace )
		return false;

	std::tstring text;
	if ( m_hWnd != NULL )
		text = GetText();

	m_visibleWhiteSpace = visibleWhiteSpace;

	if ( m_hWnd != NULL )
		SetText( text );

	return true;
}

std::tstring CTextEdit::GetSelText( void ) const
{
	Range< int > sel;
	GetSel( sel.m_start, sel.m_end );
	return ui::GetWindowText( this ).substr( sel.m_start, sel.GetSpan< size_t >() );
}

Range< int > CTextEdit::GetLineRangeAt( int charPos /*= CaretPos*/ ) const
{
	int linePos = LineFromChar( charPos );
	Range< int > lineRange( LineIndex( linePos ) );
	lineRange.m_end += LineLength( lineRange.m_start );
	return lineRange;
}

std::tstring CTextEdit::GetLineTextAt( int charPos /*= CaretPos*/ ) const
{
	TCHAR line[ 1024 ];
	int linePos = LineFromChar( charPos );
	int len = GetLine( linePos, line, COUNT_OF( line ) );
	line[ len ] = _T('\0');
	return line;
}

CFont* CTextEdit::GetFixedFont( FontSize fontSize /*= Normal*/ )
{
	static CFont fixedFont[ 2 ];
	if ( NULL == fixedFont[ fontSize ].GetSafeHandle() )
		ui::MakeStandardControlFont( fixedFont[ fontSize ], ui::CFontInfo( _T("Consolas"), ui::Regular, Normal == fontSize ? 100 : 120 ) );		// "Courier New"
	return &fixedFont[ fontSize ];
}

void CTextEdit::SetFixedFont( CWnd* pWnd )
{
	pWnd->SetFont( GetFixedFont() );
	if ( CEdit* pEdit = dynamic_cast< CEdit* >( pWnd ) )
		pEdit->SetTabStops( 16 );				// default tab stop is 32 dialog base units (8 chars), reduce it to half (4 chars)
	pWnd->Invalidate();
}

bool CTextEdit::IsInternalChange( void ) const
{
	if ( m_userChange.IsInternalChange() )
		return false;							// forced user change - allows derived classes to use SetText()

	return CInternalChange::IsInternalChange();
}

void CTextEdit::OnValueChanged( void )
{
}

void CTextEdit::PreSubclassWindow( void )
{
	BaseClass::PreSubclassWindow();

	if ( m_useFixedFont )
		SetFixedFont( this );
}

BOOL CTextEdit::PreTranslateMessage( MSG* pMsg )
{
	return m_accel.Translate( pMsg, m_hWnd ) || CEdit::PreTranslateMessage( pMsg );
}


BEGIN_MESSAGE_MAP( CTextEdit, BaseClass )
	ON_WM_GETDLGCODE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CONTROL_REFLECT_EX( EN_CHANGE, OnEnChange_Reflect )
	ON_CONTROL_REFLECT_EX( EN_KILLFOCUS, OnEnKillFocus_Reflect )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnSelectAll )
END_MESSAGE_MAP()

UINT CTextEdit::OnGetDlgCode( void )
{
	UINT code = BaseClass::OnGetDlgCode();
	if ( m_keepSelOnFocus )
		ClearFlag( code, DLGC_HASSETSEL );
	return code;
}

void CTextEdit::OnHScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	__super::OnHScroll( sbCode, pos, pScrollBar );
	if ( m_hookThumbTrack && SB_THUMBTRACK == sbCode )
		ui::SendCommandToParent( m_hWnd, EN_HSCROLL );
}

void CTextEdit::OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	__super::OnVScroll( sbCode, pos, pScrollBar );
	if ( m_hookThumbTrack && SB_THUMBTRACK == sbCode )
		ui::SendCommandToParent( m_hWnd, EN_VSCROLL );
}

BOOL CTextEdit::OnEnChange_Reflect( void )
{
	if ( IsInternalChange() ||		// doesn't propagate internal changes
		 HasInvalidText() )			// ignore invalid input text (partial input)
		return TRUE;				// skip parent routing

	OnValueChanged();
	return FALSE;					// continue routing
}

BOOL CTextEdit::OnEnKillFocus_Reflect( void )
{
	NormalizeText();
	return 0;
}

void CTextEdit::OnSelectAll( void )
{
	SelectAll();
}


// CImageEdit implementation

CImageEdit::CImageEdit( void )
	: CTextEdit( false )			// no fixed font
	, m_pImageList( NULL )
	, m_imageIndex( -1 )
	, m_imageSize( 0, 0 )
	, m_imageNcRect( 0, 0, 0, 0 )
{
}

CImageEdit::~CImageEdit()
{
}

void CImageEdit::SetImageList( CImageList* pImageList )
{
	m_pImageList = pImageList;
	if ( m_pImageList != NULL )
		m_imageSize = gdi::GetImageSize( *m_pImageList );
}

bool CImageEdit::SetImageIndex( int imageIndex )
{
	if ( m_imageIndex == imageIndex )
		return false;

	m_imageIndex = imageIndex;
	if ( ( NULL == m_pImageList ) == !m_imageNcRect.IsRectEmpty() )
		ResizeNonClient();
	else
		ui::RedrawControl( m_hWnd );
	return true;
}

void CImageEdit::ResizeNonClient( void )
{
	SetWindowPos( NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER );
}

void CImageEdit::DrawImage( CDC* pDC, const CRect& imageRect )
{
	ASSERT_PTR( m_pImageList );
	m_pImageList->DrawEx( pDC, m_imageIndex, imageRect.TopLeft(), imageRect.Size(), CLR_NONE, pDC->GetBkColor(), ILD_TRANSPARENT );
}

void CImageEdit::PreSubclassWindow( void )
{
	CTextEdit::PreSubclassWindow();
	if ( m_pImageList != NULL )
		ResizeNonClient();
}


BEGIN_MESSAGE_MAP( CImageEdit, CTextEdit )
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_MOVE()
END_MESSAGE_MAP()

void CImageEdit::OnNcCalcSize( BOOL calcValidRects, NCCALCSIZE_PARAMS* pNcSp )
{
	CTextEdit::OnNcCalcSize( calcValidRects, pNcSp );

	if ( calcValidRects )
		if ( HasValidImage() )
		{
			RECT* pClientNew = &pNcSp->rgrc[ 0 ];		// in parent's client coordinates

			m_imageNcRect = *pClientNew;
			m_imageNcRect.right = m_imageNcRect.left + m_imageSize.cx + ImageSpacing * 2 + ImageToTextGap;
			pClientNew->left = m_imageNcRect.right;
		}
		else
			m_imageNcRect.SetRectEmpty();
}

void CImageEdit::OnNcPaint( void )
{
	CTextEdit::OnNcPaint();

	if ( HasValidImage() && !m_imageNcRect.IsRectEmpty() )
	{
		CRect ncRect = m_imageNcRect;
		CWnd* pParent = GetParent();

		pParent->ClientToScreen( &ncRect );
		ui::ScreenToNonClient( m_hWnd, ncRect ); // this edit non-client coordinates

		CRect imageRect = CRect( ncRect.TopLeft(), m_imageSize );
		imageRect.OffsetRect( ImageSpacing, 0 );

		CWindowDC dc( this );
		HBRUSH hBkBrush = (HBRUSH)pParent->SendMessage( IsWritable() ? WM_CTLCOLOREDIT : WM_CTLCOLORSTATIC, (WPARAM)dc.m_hDC, (LPARAM)m_hWnd );

		::FillRect( dc, &ncRect, hBkBrush );
		DrawImage( &dc, imageRect );
	}
}

LRESULT CImageEdit::OnNcHitTest( CPoint point )
{
	if ( !m_imageNcRect.IsRectNull() )
	{
		CPoint parentPoint = point;
		GetParent()->ScreenToClient( &parentPoint );

		if ( m_imageNcRect.PtInRect( parentPoint ) )
			return HTOBJECT; // so that it will send a WM_NCLBUTTONDBLCLK
	}

	return CTextEdit::OnNcHitTest( point );
}

void CImageEdit::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	CTextEdit::OnNcLButtonDown( hitTest, point );
}

void CImageEdit::OnMove( int x, int y )
{
	CTextEdit::OnMove( x, y );
	ResizeNonClient();
}
