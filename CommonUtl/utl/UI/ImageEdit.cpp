
#include "stdafx.h"
#include "ImageEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
		m_imageSize = gdi::GetImageIconSize( *m_pImageList );
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
	__super::PreSubclassWindow();

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
	__super::OnNcCalcSize( calcValidRects, pNcSp );

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
	__super::OnNcPaint();

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

	return __super::OnNcHitTest( point );
}

void CImageEdit::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	__super::OnNcLButtonDown( hitTest, point );
}

void CImageEdit::OnMove( int x, int y )
{
	__super::OnMove( x, y );
	ResizeNonClient();
}
