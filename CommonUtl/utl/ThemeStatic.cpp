
#include "stdafx.h"
#include "ThemeStatic.h"
#include "MenuUtilities.h"
#include "ScopedGdi.h"
#include "UtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CThemeStatic implementation

CThemeStatic::CThemeStatic( const CThemeItem& bkgndItem, const CThemeItem& contentItem /*= CThemeItem::m_null*/ )
	: CBufferedStatic()
	, m_bkgndItem( bkgndItem )
	, m_contentItem( contentItem )
	, m_textSpacing( 3, 0 )
	, m_useText( false )
	, m_state( Normal )
{
}

CThemeStatic::~CThemeStatic()
{
}

bool CThemeStatic::HasCustomFacet( void ) const
{
	return true;		// if !IsThemed() it may still have fallback drawing
}

bool CThemeStatic::SetState( State state )
{
	if ( m_state == state )
		return false;

	m_state = state;
	Invalidate();
	return true;
}

void CThemeStatic::Draw( CDC* pDC, const CRect& clientRect )
{
	CThemeItem::Status drawStatus = GetDrawStatus();

	bool bkDrawn = false;
	if ( m_bkgndItem.DrawStatusBackground( drawStatus, *pDC, clientRect ) )
		bkDrawn = true;
	if ( m_contentItem.DrawStatusBackground( drawStatus, *pDC, clientRect ) )
		bkDrawn = true;

	if ( !bkDrawn )			// draw a raised frame fallback background
		pDC->DrawEdge( const_cast< CRect* >( &clientRect ), BDR_RAISEDINNER, BF_RECT | BF_SOFT );

	CRect textBounds = clientRect;
	textBounds.DeflateRect( m_textSpacing );
	DrawTextContent( pDC, textBounds, drawStatus );
}

void CThemeStatic::DrawTextContent( CDC* pDC, const CRect& textBounds, CThemeItem::Status drawStatus )
{
	if ( m_useText )
		DrawFallbackText( GetTextThemeItem(), drawStatus, pDC, const_cast< CRect& >( textBounds ), ui::GetWindowText( this ), GetDrawTextFlags() );
}

void CThemeStatic::DrawFallbackText( const CThemeItem* pTextTheme, CThemeItem::Status drawStatus, CDC* pDC, CRect& rRect,
									 const std::tstring& text, UINT dtFlags, CFont* pFallbackFont /*= NULL*/ ) const
{
	CRect anchorRect = rRect;
	if ( NULL == pTextTheme || !pTextTheme->DrawStatusText( drawStatus, *pDC, rRect, text.c_str(), dtFlags ) )
	{
		CScopedDrawText scopedDrawText( pDC, this, pFallbackFont != NULL ? pFallbackFont : GetFont() );
		pDC->DrawText( text.c_str(), (int)text.length(), &rRect, dtFlags );
	}

	if ( HasFlag( dtFlags, DT_CALCRECT ) )
		if ( !anchorRect.IsRectNull() && !rRect.IsRectEmpty() )
			ui::AlignRect( rRect, anchorRect, ui::GetDrawTextAlignment( dtFlags ) );		// also align to initial anchor
}

CSize CThemeStatic::ComputeIdealTextSize( void )
{
	CRect textBounds;
	GetClientRect( &textBounds );

	if ( m_useText )
	{
		std::tstring text = ui::GetWindowText( this );
		CClientDC clientDC( this );

		textBounds.DeflateRect( m_textSpacing );
		DrawFallbackText( GetTextThemeItem(), GetDrawStatus(), &clientDC, textBounds, text, GetDrawTextFlags() | DT_CALCRECT, GetFont() );		// compute whole text size (aligned to textBounds)
		textBounds.InflateRect( m_textSpacing );
	}

	CSize idealSize = textBounds.Size() + ui::GetNonClientSize( m_hWnd );
	return idealSize;
}

CThemeItem::Status CThemeStatic::GetDrawStatus( void ) const
{
	if ( HasFlag( GetStyle(), WS_DISABLED ) )
		return CThemeItem::Disabled;

	switch ( m_state )
	{
		case Hot:		return CThemeItem::Hot;
		case Pressed:	return CThemeItem::Pressed;
	}
	return CThemeItem::Normal;
}

CThemeItem* CThemeStatic::GetTextThemeItem( void )
{
	if ( m_useText )
		if ( m_textItem.IsValid() )
			return &m_textItem;
		else if ( m_contentItem.IsValid() )
			return &m_contentItem;
		else
			return &m_bkgndItem;

	return NULL;
}


BEGIN_MESSAGE_MAP( CThemeStatic, CBufferedStatic )
	ON_WM_MOUSEMOVE()
	ON_MESSAGE( WM_MOUSELEAVE, OnMouseLeave )
END_MESSAGE_MAP()

void CThemeStatic::OnMouseMove( UINT flags, CPoint point )
{
	CBufferedStatic::OnMouseMove( flags, point );

	if ( UseMouseInput() && m_state != Pressed )
	{
		SetState( Hot );
		TRACKMOUSEEVENT trackInfo = { sizeof( TRACKMOUSEEVENT ), TME_LEAVE, m_hWnd, HOVER_DEFAULT };
		::_TrackMouseEvent( &trackInfo );
	}
}

LRESULT CThemeStatic::OnMouseLeave( WPARAM, LPARAM )
{
	if ( UseMouseInput() && m_state != Pressed )
		SetState( Normal );
	return Default();				// process default animations
}


// CRegularStatic implementation

CRegularStatic::CRegularStatic( Style style /*= Static*/ )
	: CThemeStatic( CThemeItem( L"EDIT", EP_BACKGROUND, EBS_DISABLED ) )
{
	m_useText = true;
	m_textSpacing.cx = 0;
	SetStyle( style );
}

void CRegularStatic::SetStyle( Style style )
{
	switch ( style )
	{
		default: ASSERT( false );
		case Static:
			m_textItem = CThemeItem( L"EDIT", EP_EDITTEXT, ETS_NORMAL );
			break;
		case Bold:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_BODYTITLE );
			break;
		case Instruction:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_INSTRUCTION );
			break;
		case ControlLabel:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_CONTROLLABEL, TS_CONTROLLABEL_NORMAL );
			break;
		case Hyperlink:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_HYPERLINKTEXT, TS_HYPERLINK_HOT );
			break;
	}
}


// CHeadlineStatic implementation

CHeadlineStatic::CHeadlineStatic( Style style /*= MainInstruction*/ )
	: CThemeStatic( CThemeItem( L"EDIT", EP_EDITTEXT, ETS_NORMAL ) )
{
	m_useText = true;
	m_textSpacing.cx = 8;		// more spacing from the left edge
	SetStyle( style );
}

void CHeadlineStatic::SetStyle( Style style )
{
	switch ( style )
	{
		default: ASSERT( false );
		case MainInstruction:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_MAININSTRUCTION );
			break;
		case Instruction:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_INSTRUCTION );
			break;
		case BoldTitle:
			m_textItem = CThemeItem( L"TEXTSTYLE", TEXT_BODYTITLE );
			break;
	}
}


// CStatusStatic implementation

CStatusStatic::CStatusStatic( Style style /*= Recessed*/ )
	: CThemeStatic( CThemeItem::m_null )
{
	m_useText = true;
	SetStyle( style );
}

void CStatusStatic::SetStyle( Style style )
{
	static CThemeItem s_textItem( L"CONTROLPANEL", CPANEL_HELPLINK, CPHL_NORMAL );

	switch ( style )
	{
		default: ASSERT( false );
		case Recessed:
			m_bkgndItem = CThemeItem( L"EXPLORERBAR", EBP_NORMALGROUPBACKGROUND, 0 );
			m_textItem = CThemeItem::m_null;			// use m_bkgndItem for text
			break;
		case Rounded:
			m_bkgndItem = CThemeItem( L"LISTVIEW", LVP_GROUPHEADER, LVGH_CLOSESELECTEDNOTFOCUSED );
			m_textItem = s_textItem;
			break;
		case RoundedHot:
			m_bkgndItem = CThemeItem( L"LISTVIEW", LVP_GROUPHEADER, LVGH_OPENHOT );
			m_textItem = s_textItem;
			break;
	}

	if ( !m_bkgndItem.IsThemed() && style != Rounded )
		SetStyle( Rounded );
}


// CPickMenuStatic implementation

CPickMenuStatic::CPickMenuStatic( ui::PopupAlign popupAlign /*= ui::DropRight*/ )
	: CThemeStatic( CThemeItem( L"SCROLLBAR", vt::SBP_THUMBBTNHORZ, vt::SCRBS_NORMAL ), CThemeItem( L"HEADER", vt::HP_HEADEROVERFLOW, vt::HOFS_NORMAL ) )
	, m_popupAlign( popupAlign )		// to match HP_HEADEROVERFLOW pointing right
{
	m_bkgndItem
		.SetStateId( CThemeItem::Hot, vt::SCRBS_HOT )
		.SetStateId( CThemeItem::Pressed, vt::SCRBS_PRESSED );

	m_textItem = CThemeItem( L"BUTTON", vt::BP_PUSHBUTTON, vt::PBS_NORMAL );
	m_textItem.SetStateId( CThemeItem::Disabled, vt::PBS_DISABLED );

	if ( m_popupAlign != ui::DropRight )
		SetPopupAlign( m_popupAlign );
}

CPickMenuStatic::~CPickMenuStatic()
{
}

void CPickMenuStatic::SetPopupAlign( ui::PopupAlign popupAlign )
{
	m_popupAlign = popupAlign;
	m_contentItem = CThemeItem::m_null;			// reset content item to display an arrow
}

void CPickMenuStatic::DrawTextContent( CDC* pDC, const CRect& textBounds, CThemeItem::Status drawStatus )
{
	if ( m_contentItem.IsThemed() )
		return;				// use the themed content

	static const TCHAR alignArrows[] = _T("\x34\x36\x33\x35");		// indexed by ui::PopupAlign
	static const TCHAR arrowSpace[] = _T("   ");					// approximates the spaced arrow

	UINT dtFlags = GetDrawTextFlags();
	CFont* pMarlettFont = GetMarlettFont();
	std::tstring wholeText;

	if ( m_useText )
		wholeText = ui::GetWindowText( this );

	CRect wholeRect = textBounds;
	if ( wholeText.empty() )
	{
		CScopedDrawText scopedDrawText( pDC, this, pMarlettFont );
		pDC->DrawText( alignArrows + m_popupAlign, 1, &wholeRect, dtFlags );
		return;		// done with just the arrow
	}
	else if ( ui::DropLeft == m_popupAlign )
		wholeText.insert( 0, arrowSpace );		// "< text"
	else
		wholeText += arrowSpace;				// "text >"

	CThemeItem* pTextTheme = GetTextThemeItem();
	CFont* pTextFont = GetFont();

	// compute whole text size (aligned to textBounds)
	DrawFallbackText( pTextTheme, drawStatus, pDC, wholeRect, wholeText, dtFlags | DT_CALCRECT, pTextFont );

	// draw whole text
	DrawFallbackText( pTextTheme, drawStatus, pDC, wholeRect, wholeText, dtFlags, pTextFont );

	// draw arrow text
	{
		UINT dtArrowFlags = dtFlags;
		ModifyFlag( dtArrowFlags, DT_CENTER | DT_RIGHT, ui::DropLeft == m_popupAlign ? DT_LEFT : DT_RIGHT );
		wholeRect.InflateRect( ArrowDelta, 0 );		// space the arrow more

		CScopedDrawText scopedDrawText( pDC, this, pMarlettFont );
		pDC->DrawText( alignArrows + m_popupAlign, 1, &wholeRect, dtArrowFlags );
	}
}

void CPickMenuStatic::TrackMenu( CWnd* pTargetWnd, CMenu* pPopupMenu )
{
	ASSERT_PTR( pPopupMenu );

	CRect pickerRect;
	GetWindowRect( &pickerRect );
	SetState( CThemeStatic::Pressed );
	ui::TrackPopupMenuAlign( *pPopupMenu, pTargetWnd, pickerRect, m_popupAlign, 0 );
	SetState( CThemeStatic::Normal );
}

void CPickMenuStatic::TrackMenu( CWnd* pTargetWnd, UINT menuId, int popupIndex, bool useCheckedBitmaps /*= false*/ )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, menuId, popupIndex, useCheckedBitmaps ? ui::CheckedMenuImages : ui::NormalMenuImages );
	TrackMenu( pTargetWnd, &popupMenu );
}


// CDetailsButton implementation

CDetailsButton::CDetailsButton( void )
	: CThemeStatic( CThemeItem( L"SCROLLBAR", vt::SBP_THUMBBTNHORZ, vt::SCRBS_NORMAL ), CThemeItem( L"SCROLLBAR", vt::SBP_GRIPPERVERT, vt::SCRBS_NORMAL ) )
{
	m_bkgndItem
		.SetStateId( CThemeItem::Hot, vt::SCRBS_HOT )
		.SetStateId( CThemeItem::Pressed, vt::SCRBS_PRESSED );

	m_contentItem
		.SetStateId( CThemeItem::Hot, vt::SCRBS_HOT )
		.SetStateId( CThemeItem::Pressed, vt::SCRBS_PRESSED );
}

CDetailsButton::~CDetailsButton()
{
}


// CLinkStatic implementation

CLinkStatic::CLinkStatic( const TCHAR* pLinkPrefix /*= NULL*/ )
	: CThemeStatic( CThemeItem( L"CONTROLPANEL", vt::CPANEL_CONTENTLINK, vt::CPCL_NORMAL ) )
	, m_linkPrefix( pLinkPrefix )
{
	m_useText = true;
	m_bkgndItem
		.SetStateId( CThemeItem::Hot, vt::CPCL_HOT )
		.SetStateId( CThemeItem::Pressed, vt::CPCL_PRESSED )
		.SetStateId( CThemeItem::Disabled, vt::CPCL_DISABLED );
}

CLinkStatic::~CLinkStatic()
{
}

std::tstring CLinkStatic::GetLinkText( void ) const
{
	return m_linkPrefix + ui::GetWindowText( this );
}

bool CLinkStatic::Navigate( void )
{
	std::tstring hyperLink = GetLinkText();

	return
		!hyperLink.empty() &&
		::ShellExecute( m_hWnd, _T("open"), hyperLink.c_str(), 0, 0, SW_SHOWNORMAL ) > (HINSTANCE)HINSTANCE_ERROR;
}

BEGIN_MESSAGE_MAP( CLinkStatic, CThemeStatic )
	ON_WM_NCHITTEST()
	ON_WM_SETCURSOR()
	ON_CONTROL_REFLECT_EX( BN_CLICKED, OnBnClicked_Reflect )
END_MESSAGE_MAP()

LRESULT CLinkStatic::OnNcHitTest( CPoint point )
{
	point;
	return HTCLIENT;
}

BOOL CLinkStatic::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	static HCURSOR hLinkCursor = AfxGetApp()->LoadStandardCursor( IDC_HAND );
	if ( hLinkCursor != NULL )
	{
		::SetCursor( hLinkCursor );
		return TRUE;
	}
	return CThemeStatic::OnSetCursor( pWnd, hitTest, message );
}

BOOL CLinkStatic::OnBnClicked_Reflect( void )
{
	if ( !Navigate() )
		ui::BeepSignal( MB_ICONERROR );
	return TRUE;	// handled
}
