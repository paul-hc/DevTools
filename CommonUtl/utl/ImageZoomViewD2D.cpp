
#include "stdafx.h"
#include "ImageZoomViewD2D.h"
#include "ImagingWic.h"
#include "AnimatedFrameComposer.h"
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
		, m_animTimer( m_pZoomView, AnimateTimer, 1000 )
	{
		ASSERT_PTR( m_pZoomView->GetSafeHwnd() );
	}

	CImageRenderTarget::~CImageRenderTarget()
	{
	}

	void CImageRenderTarget::DiscardResources( void )
	{
		CWindowRenderTarget::DiscardResources();

		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->Reset();
	}

	bool CImageRenderTarget::CreateResources( void )
	{
		if ( !CWindowRenderTarget::CreateResources() )
			return false;

		return NULL == m_pAnimComposer.get() || m_pAnimComposer->Create();
	}

	CWicImage* CImageRenderTarget::GetImage( void ) const
	{
		return m_pZoomView->GetImage();
	}

	CWicAnimatedImage* CImageRenderTarget::GetAnimImage( void ) const
	{
		if ( CWicImage* pImage = GetImage() )
			if ( pImage->IsAnimated() )
				return checked_static_cast< CWicAnimatedImage* >( pImage );

		return NULL;
	}

	bool CImageRenderTarget::HasAnimatedImage( void )
	{
		CWicImage* pImage = GetImage();
		return pImage != NULL && pImage->IsAnimated();
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
			if ( NULL == m_pAnimComposer.get() || !m_pAnimComposer->HasImage( pImage ) )
			{
				m_pAnimComposer.reset( new CAnimatedFrameComposer( m_pZoomView, checked_static_cast< CWicAnimatedImage* >( pImage ), &m_animTimer ) );
				m_pAnimComposer->Create();
			}
		}
		else
		{
			m_pAnimComposer.reset();
			m_animTimer.Stop();
		}
	}

	bool CImageRenderTarget::DrawImage( const CDrawBitmapTraits& traits )
	{
		SetupCurrentImage();
		if ( !CanDraw() )
			return false;				// window is occluded

		CScopedDraw scopedDraw( this );

		ClearBackground( m_pZoomView->GetBkColor() );

		if ( m_pAnimComposer.get() != NULL )
			return RenderDone == m_pAnimComposer->DrawBitmap( traits, m_pZoomView->GetContentRect() );		// draw current animated frame

		return RenderDone == DrawBitmap( traits, m_pZoomView->GetContentRect() );							// draw static image
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

void CImageZoomViewD2D::PrintImageGdi( CDC* pPrnDC, CWicImage* pImage )
{
	REQUIRE( pPrnDC->IsPrinting() );
	ASSERT_PTR( pImage );

	::SetBrushOrgEx( pPrnDC->m_hDC, 0, 0, NULL );

	CRect prnPageRect( 0, 0, pPrnDC->GetDeviceCaps( HORZRES ), pPrnDC->GetDeviceCaps( VERTRES ) );		// printer page rect in pixels
	CSize prnDpi( pPrnDC->GetDeviceCaps( LOGPIXELSX ), pPrnDC->GetDeviceCaps( LOGPIXELSY ) );			// printer resolution in pixels per inch

	CSize prnBmpSize = GetContentRect().Size();
	prnBmpSize.cx *= prnDpi.cx;
	prnBmpSize.cy *= prnDpi.cy;

	// best fit image in printer page - create a rectangle which preserves the image's aspect ratio, and fills the page horizontally
	CSize prnDisplaySize = ui::StretchToFit( prnPageRect.Size(), prnBmpSize );

	CRect displayRect( 0, 0, prnDisplaySize.cx, prnDisplaySize.cy );
	ui::CenterRect( displayRect, prnPageRect );

	CWicDibSection dibSection( pImage->GetWicBitmap() );
	dibSection.Draw( pPrnDC, displayRect, ui::StretchFit );
}

void CImageZoomViewD2D::OnDraw( CDC* pDC )
{
	if ( CWicImage* pImage = GetImage() )
		if ( pDC->IsPrinting() )
			PrintImageGdi( pDC, pImage );
		else if ( m_pImageRT.get() != NULL )
		{
			m_pImageRT->EnsureResources();

			if ( m_pImageRT->IsValid() )
			{
				// apply translation transform according to view's scroll position
				CPoint point = GetScrollPosition();
				m_drawTraits.m_transform = D2D1::Matrix3x2F::Translation( (float)-point.x, (float)-point.y );

				m_drawTraits.SetAutoInterpolationMode( GetContentRect().Size(), GetSourceSize() );

				m_pImageRT->DrawImage( m_drawTraits );
			}
		}
}


// message handlers

BEGIN_MESSAGE_MAP( CImageZoomViewD2D, CBaseZoomView )
	ON_WM_CREATE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

int CImageZoomViewD2D::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CBaseZoomView::OnCreate( pCreateStruct ) )
		return -1;

	m_pImageRT.reset( new d2d::CImageRenderTarget( this ) );
	m_pImageRT->CreateResources();
	return 0;
}

void CImageZoomViewD2D::OnSize( UINT sizeType, int cx, int cy )
{
	CBaseZoomView::OnSize( sizeType, cx, cy );

	if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
		if ( IsValidRenderTarget() )
			m_pImageRT->Resize( CSize( cx, cy ) );
}

void CImageZoomViewD2D::OnTimer( UINT_PTR eventId )
{
	if ( m_pImageRT.get() != NULL && m_pImageRT->IsAnimEvent( eventId ) )
		m_pImageRT->HandleAnimEvent();
	else
		CBaseZoomView::OnTimer( eventId );
}
