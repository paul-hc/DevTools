
#include "stdafx.h"
#include "BufferedStatic.h"
#include "MemoryDC.h"
#include "ScopedGdi.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBufferedStatic::CBufferedStatic( void )
	: CStatic()
	, CContentFitBase( this )
	, m_dtFlags( UINT_MAX )
{
}

CBufferedStatic::~CBufferedStatic()
{
}

std::tstring CBufferedStatic::GetWindowText( void ) const
{
	std::tstring text;
	ui::GetWindowText( text, m_hWnd );
	return text;
}

bool CBufferedStatic::SetWindowText( const std::tstring& text )
{
	if ( !ui::SetWindowText( m_hWnd, text ) )
		return false;			// text hasn not changed

	OnContentChanged();
	return true;
}

UINT CBufferedStatic::GetDrawTextFlags( void ) const
{
	return m_dtFlags;
}

void CBufferedStatic::PaintImpl( CDC* pDC, const CRect& clientRect )
{
	CScopedGdi< CBrush > scopedBrush( pDC, CBrush::FromHandle( ui::SendCtlColor( m_hWnd, *pDC, WM_CTLCOLORSTATIC ) ) );
	CScopedGdi< CFont > scopedFont( pDC, GetFont() );

	DrawBackground( pDC, clientRect );
	Draw( pDC, clientRect );
}

void CBufferedStatic::DrawBackground( CDC* pDC, const CRect& clientRect )
{
	pDC->PatBlt( clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(), PATCOPY );		// erase background
}

CFont* CBufferedStatic::GetMarlettFont( void )
{
	static CFont marlettFont;
	if ( nullptr == marlettFont.GetSafeHandle() )
		ui::MakeStandardControlFont( marlettFont, ui::CFontInfo( _T("Marlett"), ui::Regular, 120 ) );
	return &marlettFont;
}

void CBufferedStatic::PreSubclassWindow( void )
{
	CStatic::PreSubclassWindow();

	DWORD style = GetStyle();			// get prior to modify

	if ( HasCustomFacet() )				// avoid making it owner drawn if themes are disabled
		ModifyStyle( SS_TYPEMASK, SS_OWNERDRAW );

	if ( UINT_MAX == m_dtFlags )
	{
		m_dtFlags = DT_LEFT | DT_TOP /*| DT_NOPREFIX*/;

		switch ( style & SS_TYPEMASK )
		{
			case SS_CENTER:	m_dtFlags |= DT_CENTER; break;
			case SS_RIGHT:	m_dtFlags |= DT_RIGHT; break;
		}

		switch ( style & SS_ELLIPSISMASK )
		{
			case SS_ENDELLIPSIS:	m_dtFlags |= DT_END_ELLIPSIS; break;
			case SS_PATHELLIPSIS:	m_dtFlags |= DT_PATH_ELLIPSIS; break;
			case SS_WORDELLIPSIS:	m_dtFlags |= DT_WORD_ELLIPSIS; break;
		}

		if ( HasFlag( style, SS_CENTERIMAGE ) )
			m_dtFlags |= DT_VCENTER | DT_SINGLELINE;

		if ( HasFlag( style, SS_NOPREFIX ) )
			m_dtFlags |= DT_NOPREFIX;
	}
}

void CBufferedStatic::DrawItem( DRAWITEMSTRUCT* pDrawItem )
{
	switch ( pDrawItem->itemAction )
	{
		case ODA_DRAWENTIRE:
		case ODA_SELECT:
			break;
		case ODA_FOCUS:
			if ( UseMouseInput() )
				break;
			// fall-through
		default:
			return;
	}

	CRect clientRect = pDrawItem->rcItem;
	CDC* pDC = CDC::FromHandle( pDrawItem->hDC );
	CMemoryDC memDC( *pDC, clientRect );					// minimize background flicker
	PaintImpl( &memDC.GetDC(), clientRect );
}


// message handlers

BEGIN_MESSAGE_MAP( CBufferedStatic, CStatic )
END_MESSAGE_MAP()
