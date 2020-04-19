
#include "stdafx.h"
#include "ImageZoomViewD2D.h"
#include "ImagingWic.h"
#include "AnimatedFrameComposer.h"
#include "Color.h"
#include "Utilities.h"
#include "WicAnimatedImage.h"
#include "WicDibSection.h"			// for printing

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CImageRenderTarget implementation

	CImageRenderTarget::CImageRenderTarget( CImageZoomViewD2D* pZoomView )
		: CWindowRenderTarget( pZoomView )
		, m_pZoomView( pZoomView )
		, m_accentFrameColor( ::GetSysColor( COLOR_ACTIVECAPTION ) )
		, m_animTimer( m_pZoomView, AnimateTimer, 1000 )
	{
		ASSERT_PTR( m_pZoomView->GetSafeHwnd() );
	}

	CImageRenderTarget::~CImageRenderTarget()
	{
	}

	CWicImage* CImageRenderTarget::GetImage( void ) const
	{
		return m_pZoomView->GetImage();
	}

	void CImageRenderTarget::HandleAnimEvent( void )
	{
		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->HandleAnimEvent();
	}

	void CImageRenderTarget::SetupCurrentImage( void )
	{
		CWicImage* pImage = GetImage();
		SetWicBitmap( pImage != NULL ? pImage->GetWicBitmap() : NULL );

		if ( pImage != NULL && pImage->IsAnimated() )
		{
			if ( NULL == m_pAnimComposer.get() || !m_pAnimComposer->UsesImage( pImage ) )
			{
				m_pAnimComposer.reset( new CAnimatedFrameComposer( this, checked_static_cast< CWicAnimatedImage* >( pImage ) ) );
				m_pAnimComposer->Create();
			}
		}
		else
		{
			m_pAnimComposer.reset();
			m_animTimer.Stop();
		}
	}

	void CImageRenderTarget::DiscardResources( void )
	{
		CWindowRenderTarget::DiscardResources();

		m_pAccentFrameBrush = NULL;

		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->Reset();
	}

	bool CImageRenderTarget::CreateResources( void )
	{
		if ( !CWindowRenderTarget::CreateResources() )
			return false;

		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
		{
			if ( m_accentFrameColor != CLR_NONE )
				HR_AUDIT( pRenderTarget->CreateSolidColorBrush( ToColor( m_accentFrameColor, 50 ), (ID2D1SolidColorBrush**)&m_pAccentFrameBrush ) );
		}

		return NULL == m_pAnimComposer.get() || m_pAnimComposer->Create();
	}

	void CImageRenderTarget::StartAnimation( UINT frameDelay )
	{
		m_animTimer.Start( frameDelay );
	}

	void CImageRenderTarget::StopAnimation( void )
	{
		m_animTimer.Stop();
	}

	void CImageRenderTarget::DrawBitmap( const CViewCoords& coords )
	{
		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->DrawBitmap( coords );			// draw current animated frame
		else
			__super::DrawBitmap( coords );					// draw static image
	}

	void CImageRenderTarget::PreDraw( const CViewCoords& coords )
	{
		coords;
		ClearBackground( m_pZoomView->GetBkColor() );
	}

	void CImageRenderTarget::PostDraw( const CViewCoords& coords )
	{
		D2D_RECT_F destRectF = d2d::ToRectF( coords.m_contentRect );

		if ( m_pZoomView->IsAccented() && m_pAccentFrameBrush != NULL )
			GetRenderTarget()->DrawRectangle( &destRectF, m_pAccentFrameBrush, 5 );		// draw the outline
	}


	bool CImageRenderTarget::DrawImage( const CViewCoords& coords )
	{
		if ( !CanRender() )
			return false;				// window is occluded

		SetupCurrentImage();

		return RenderDone == Render( coords );
	}

} //namespace d2d


// CImageZoomViewD2D implementation

CImageZoomViewD2D::CImageZoomViewD2D( void )
	: CBaseZoomView( ui::ActualSize, 100 )
{
}

CImageZoomViewD2D::~CImageZoomViewD2D()
{
}

CSize CImageZoomViewD2D::GetSourceSize( void ) const
{
	CWicImage* pImage = GetImage();
	return pImage != NULL ? pImage->GetBmpSize() : CSize( 0, 0 );
}

bool CImageZoomViewD2D::IsAccented( void ) const
{
	return false;
}

void CImageZoomViewD2D::PrintImageGdi( CDC* pPrintDC, CWicImage* pImage )
{
	REQUIRE( pPrintDC->IsPrinting() );
	ASSERT_PTR( pImage );

	::SetBrushOrgEx( pPrintDC->m_hDC, 0, 0, NULL );

	CRect prnPageRect( 0, 0, pPrintDC->GetDeviceCaps( HORZRES ), pPrintDC->GetDeviceCaps( VERTRES ) );		// printer page rect in pixels
	CSize prnDpi( pPrintDC->GetDeviceCaps( LOGPIXELSX ), pPrintDC->GetDeviceCaps( LOGPIXELSY ) );			// printer resolution in pixels per inch

	CSize prnBmpSize = GetContentRect().Size();
	prnBmpSize.cx *= prnDpi.cx;
	prnBmpSize.cy *= prnDpi.cy;

	// best fit image in printer page - create a rectangle which preserves the image's aspect ratio, and fills the page horizontally
	CSize prnDisplaySize = ui::StretchToFit( prnPageRect.Size(), prnBmpSize );

	CRect displayRect( 0, 0, prnDisplaySize.cx, prnDisplaySize.cy );
	ui::CenterRect( displayRect, prnPageRect );

	CWicDibSection dibSection( pImage->GetWicBitmap() );
	dibSection.Draw( pPrintDC, displayRect, ui::StretchFit );
}

void CImageZoomViewD2D::OnDraw( CDC* pDC )
{
	CWicImage* pImage = GetImage();

	if ( pDC->IsPrinting() )
	{
		if ( pImage != NULL )
			PrintImageGdi( pDC, pImage );
	}
	else if ( m_pImageRT.get() != NULL )
	{
		m_pImageRT->EnsureResources();

		if ( m_pImageRT->IsValidTarget() )
		{
			CPoint scrollPos = GetScrollPosition();

			m_drawTraits.m_transform = D2D1::Matrix3x2F::Translation( (float)-scrollPos.x, (float)-scrollPos.y );	// apply translation transform according to view's scroll position
			m_drawTraits.SetAutoInterpolationMode( GetContentRect().Size(), GetSourceSize() );

			d2d::CViewCoords viewCoords( m_drawTraits, _GetClientRect(), GetContentRect() );

			m_pImageRT->DrawImage( viewCoords );
		}
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CImageZoomViewD2D, CBaseZoomView )
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
END_MESSAGE_MAP()

int CImageZoomViewD2D::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == __super::OnCreate( pCreateStruct ) )
		return -1;

	m_pImageRT.reset( new d2d::CImageRenderTarget( this ) );
	m_pImageRT->CreateResources();
	return 0;
}

void CImageZoomViewD2D::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
		if ( IsValidRenderTarget() )
			m_pImageRT->Resize( CSize( cx, cy ) );
}

void CImageZoomViewD2D::OnTimer( UINT_PTR eventId )
{
	if ( m_pImageRT.get() != NULL && m_pImageRT->IsAnimEvent( eventId ) )
		m_pImageRT->HandleAnimEvent();
	else
		__super::OnTimer( eventId );
}

LRESULT CImageZoomViewD2D::OnDisplayChange( WPARAM bitsPerPixel, LPARAM lParam )
{
	CSize screenResolution( lParam ); screenResolution;		// just for illustration

	Invalidate();
	return __super::OnDisplayChange( bitsPerPixel, lParam );
}
