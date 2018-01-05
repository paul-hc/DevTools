
#include "stdafx.h"
#include "BaseStatic.h"
#include "MemoryDC.h"
#include "ScopedGdi.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaseStatic::CBaseStatic( void )
	: CStatic()
	, m_dtFlags( UINT_MAX )
{
}

CBaseStatic::~CBaseStatic()
{
}

void CBaseStatic::PaintImpl( CDC* pDC, const CRect& clientRect )
{
	CScopedGdi< CBrush > scopedBrush( pDC, CBrush::FromHandle( ui::SendCtlColor( m_hWnd, *pDC, WM_CTLCOLORSTATIC ) ) );
	CScopedGdi< CFont > scopedFont( pDC, GetFont() );

	DrawBackground( pDC, clientRect );
	Draw( pDC, clientRect );
}

void CBaseStatic::DrawBackground( CDC* pDC, const CRect& clientRect )
{
	pDC->PatBlt( clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(), PATCOPY );		// erase background
}

CFont* CBaseStatic::GetMarlettFont( void )
{
	static CFont marlettFont;
	if ( NULL == marlettFont.GetSafeHandle() )
		ui::MakeStandardControlFont( marlettFont, ui::CFontInfo( _T("Marlett"), false, false, 120 ) );
	return &marlettFont;
}

void CBaseStatic::PreSubclassWindow( void )
{
	CStatic::PreSubclassWindow();

	DWORD style = GetStyle();			// get prior to modify

	if ( HasCustomFacet() )				// avoid making it owner drawn if themes are disabled
		ModifyStyle( SS_TYPEMASK, SS_OWNERDRAW );

	if ( UINT_MAX == m_dtFlags )
	{
		m_dtFlags = DT_LEFT | DT_TOP | DT_NOPREFIX;

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

void CBaseStatic::DrawItem( DRAWITEMSTRUCT* pDrawItem )
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

BEGIN_MESSAGE_MAP( CBaseStatic, CStatic )
END_MESSAGE_MAP()
