
#include "pch.h"
#include "ImageEdit.h"
#include "ImageProxies.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImageEdit::CImageEdit( void )
	: CTextEdit( false )			// no fixed font
	, m_imageNcRect( 0, 0, 0, 0 )
{
}

CImageEdit::~CImageEdit()
{
}

bool CImageEdit::HasValidImage( void ) const
{
	return m_pImageProxy.get() != nullptr && !m_pImageProxy->IsEmpty();
}

bool CImageEdit::SetImageProxy( ui::IImageProxy* pImageProxy )
{
	if ( pImageProxy == m_pImageProxy.get() )
		return false;

	m_pImageProxy.reset( pImageProxy );

	UpdateControl();
	return true;
}

void CImageEdit::SetImageList( CImageList* pImageList )
{
	if ( pImageList != nullptr )
	{
		if ( nullptr == m_pImageProxy.get() || !is_a<CImageListProxy>( m_pImageProxy.get() ) )
			m_pImageProxy.reset( new CImageListProxy( pImageList ) );
		else
			GetImageProxyAs<CImageListProxy>()->SetImageList( pImageList );
	}
	else
		m_pImageProxy.reset();
}

int CImageEdit::GetImageIndex( void ) const
{
	const CImageListProxy* pImageListProxy = GetImageProxyAs<CImageListProxy>();

	ASSERT_PTR( pImageListProxy );
	return pImageListProxy->GetImageIndex();
}

bool CImageEdit::SetImageIndex( int imageIndex )
{
	if ( nullptr == m_pImageProxy.get() )
		m_pImageProxy.reset( new CImageListProxy() );		// create an empty proxy, with no image list

	CImageListProxy* pImageListProxy = GetImageProxyAs<CImageListProxy>();

	ASSERT_PTR( pImageListProxy );
	if ( !pImageListProxy->SetImageIndex( imageIndex ) )
		return false;

	UpdateControl();
	return true;
}

void CImageEdit::UpdateControl( void )
{
	if ( m_hWnd != nullptr )
		if ( (BOOL)HasValidImage() == m_imageNcRect.IsRectEmpty() )		// dirty non-client?
			ResizeNonClient();
		else
			ui::RedrawControl( m_hWnd );
}

void CImageEdit::ResizeNonClient( void )
{
	// will send a WM_NCCALCSIZE
	SetWindowPos( nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER );
}

void CImageEdit::DrawImage( CDC* pDC, const CRect& imageRect )
{
	ASSERT( HasValidImage() );
	m_pImageProxy->Draw( pDC, imageRect.TopLeft() );
}

void CImageEdit::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	UpdateControl();
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
			const CSize& imageSize = m_pImageProxy->GetSize();

			m_imageNcRect = *pClientNew;
			m_imageNcRect.right = m_imageNcRect.left + imageSize.cx + ImageSpacing * 2 + ImageToTextGap - 1;
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
		ui::ScreenToNonClient( m_hWnd, ncRect );		// this edit non-client coordinates

		CRect imageRect = CRect( ncRect.TopLeft(), m_pImageProxy->GetSize() );
		imageRect.OffsetRect( ImageSpacing + 1, 0 );

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
			return HTOBJECT;		// so that it will send a WM_NCLBUTTONDBLCLK
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
