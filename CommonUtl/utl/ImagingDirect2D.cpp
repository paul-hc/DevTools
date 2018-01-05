
#include "stdafx.h"
#include "ImagingDirect2D.h"
#include "ImagingWic.h"
#include "BaseApp.h"
#include "Utilities.h"

#pragma comment( lib, "d2d1" )					// link to Direct2D

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CImagingFactory implementation

	CImagingFactory::CImagingFactory( void )
	{
	#ifdef DEBUG_DIRECT2D
		D2D1_FACTORY_OPTIONS options;
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

		// Direct2D Debug Layer: see https://msdn.microsoft.com/en-us/library/dd940309%28VS.85%29.aspx
		HR_OK( D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &m_pFactory ) );
	#else
		HR_OK( D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory ) );
	#endif
		app::GetGlobalResources()->GetSharedResources().AddComPtr( m_pFactory );			// will release the factory singleton in ExitInstance()
	}

	CImagingFactory::~CImagingFactory()
	{
	}

	CImagingFactory& CImagingFactory::Instance( void )
	{
		static CImagingFactory factory;
		return factory;
	}


	// CDrawBitmapTraits implementation

	D2D1_BITMAP_INTERPOLATION_MODE CDrawBitmapTraits::s_enlargeInterpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;		// by default no smoothing

	void CDrawBitmapTraits::SetAutoInterpolationMode( const CSize& destBoundsSize, const CSize& bmpSize )
	{
		m_interpolationMode = ui::FitsInside( destBoundsSize, bmpSize )
			? s_enlargeInterpolationMode							// no smoothing on stretching (by default, user configurable)
			: D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;				// use smooting (dithering) when shrinking
	}

	void CDrawBitmapTraits::Draw( ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap* pBitmap, const CRect& destRect, const CRect* pSrcRect /*= NULL*/ ) const
	{
		// caller should initiate BeginDraw
		ASSERT_PTR( pRenderTarget );

		pRenderTarget->SetTransform( m_transform );

		D2D_RECT_F destRectF = d2d::ToRectF( destRect );
		if ( m_bkColor != CLR_NONE )
		{
			CComPtr< ID2D1SolidColorBrush > pBrush;
			if ( HR_OK( pRenderTarget->CreateSolidColorBrush( ToColor( m_bkColor ), &pBrush ) ) )
				pRenderTarget->FillRectangle( &destRectF, pBrush );				// clear image background
		}

		if ( pBitmap != NULL )
		{
			D2D_RECT_F srcRect = pSrcRect != NULL
				? d2d::ToRectF( *pSrcRect )
				: d2d::ToRectF( CRect( CPoint( 0, 0 ), FromSize( pBitmap->GetPixelSize() ) ) );

			pRenderTarget->DrawBitmap( pBitmap, destRectF, m_opacity, m_interpolationMode, pSrcRect != NULL ? &srcRect : NULL );
		}

		if ( m_frameColor != CLR_NONE )
		{
			CComPtr< ID2D1SolidColorBrush > pBrush;
			if ( HR_OK( pRenderTarget->CreateSolidColorBrush( ToColor( m_frameColor ), &pBrush ) ) )
				pRenderTarget->DrawRectangle( &destRectF, pBrush );				// draw a frame around the image
		}
	}


	// CRenderTarget implementation

	void CRenderTarget::OnFirstAddInternalChange( void )
	{
		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
			pRenderTarget->BeginDraw();					// for window render target: window must not be occluded
	}

	void CRenderTarget::OnFinalReleaseInternalChange( void )
	{
		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
			if ( D2DERR_RECREATE_TARGET == HR_AUDIT( pRenderTarget->EndDraw() ) )
			{
				// D2D device loss: display change, remoting, removal of video card, etc
				TRACE( _T("** Direct 2D device loss **\n") );

				DiscardResources();
				if ( CWnd* pWnd = GetWindow() )
					pWnd->Invalidate( TRUE );			// device loss: force a re-render
			}
	}

	void CRenderTarget::SetWicBitmap( IWICBitmapSource* pWicBitmap )
	{
		if ( pWicBitmap != m_pWicBitmap )
		{
			m_pWicBitmap = pWicBitmap;
			ReleaseBitmap();							// release previous D2D bitmap
		}
	}

	ID2D1Bitmap* CRenderTarget::GetBitmap( void )
	{
		if ( m_pWicBitmap != NULL && NULL == m_pBitmap )
			if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
				HR_AUDIT( pRenderTarget->CreateBitmapFromWicBitmap( m_pWicBitmap, NULL, &m_pBitmap ) );

		return m_pBitmap;
	}

	void CRenderTarget::ClearBackground( COLORREF bkColor )
	{
		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
			pRenderTarget->Clear( ToColor( bkColor ) );
	}

	RenderResult CRenderTarget::DrawBitmap( const CDrawBitmapTraits& traits, const CRect& destRect, const CRect* pSrcRect /*= NULL*/ )
	{
		EnsureResources();				// lazy render target (if not yet created or device lost)

		if ( !CanDraw() )
			return RenderError;			// window is occluded

		CScopedDraw scopedDraw( this );
		traits.Draw( GetRenderTarget(), GetBitmap(), destRect, pSrcRect );
		if ( !IsValid() )				// resources discarded?
			return DeviceLoss;			// device was lost

		return RenderDone;
	}


	// CWindowRenderTarget implementation

	bool CWindowRenderTarget::Resize( const CSize& clientSize )
	{
		if ( !HR_OK( m_pWndRenderTarget->Resize( ToSize( clientSize ) ) ) )			// IMP: if couldn't resize, release the device and we'll recreate it during the next render pass
		{
			DiscardResources();
			return false;
		}
		return true;
	}

	bool CWindowRenderTarget::Resize( void )
	{
		CRect clientRect;
		m_pWnd->GetClientRect( &clientRect );
		return Resize( clientRect.Size() );
	}

	void CWindowRenderTarget::DiscardResources( void )
	{
		m_pWndRenderTarget = NULL;
		ReleaseBitmap();
	}

	bool CWindowRenderTarget::CreateResources( void )
	{
		ASSERT_NULL( m_pWndRenderTarget );

		CRect clientRect;
		m_pWnd->GetClientRect( &clientRect );

		D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();

		// use the screen DPI to allow direct mapping between image pixels and desktop pixels in different system DPI settings
		CImagingFactory::Factory()->GetDesktopDpi( &rtProps.dpiX, &rtProps.dpiY );		// usually 96 DPI

		return HR_OK( CImagingFactory::Factory()->CreateHwndRenderTarget(
			rtProps,
			D2D1::HwndRenderTargetProperties( m_pWnd->GetSafeHwnd(), ToSize( clientRect.Size() ) ),
			&m_pWndRenderTarget ) );
	}

	bool CWindowRenderTarget::CanDraw( void ) const
	{
		return IsValid() && !HasFlag( m_pWndRenderTarget->CheckWindowState(), D2D1_WINDOW_STATE_OCCLUDED );
	}


	// CDCRenderTarget implementation

	void CDCRenderTarget::DiscardResources( void )
	{
		m_pDcRenderTarget = NULL;
		ReleaseBitmap();
	}

	bool CDCRenderTarget::CreateResources( void )
	{
		ASSERT_NULL( m_pDcRenderTarget );

		CRect clientRect;
		GetWindow()->GetClientRect( &clientRect );

		// use the screen DPI to allow direct mapping between image pixels and desktop pixels in different system DPI settings
		float dpiX, dpiY;
		CImagingFactory::Factory()->GetDesktopDpi( &dpiX, &dpiY );			// usually 96 DPI

		D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE ),			// required for GDI rendering
			dpiX, dpiY,
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_DEFAULT );

		return
			HR_OK( CImagingFactory::Factory()->CreateDCRenderTarget( &rtProps, &m_pDcRenderTarget ) ) &&
			Resize( clientRect );
	}

	bool CDCRenderTarget::Resize( const CRect& subRect )
	{
		ASSERT_PTR( m_pDC->GetSafeHdc() );

		EnsureResources();				// lazy render target (if not yet created or device lost)
		if ( !HR_OK( m_pDcRenderTarget->BindDC( m_pDC->GetSafeHdc(), &subRect ) ) )			// bind the DC to the render target
		{	// IMP: if couldn't resize, release the device and we'll recreate it during the next render pass
			DiscardResources();
			return false;
		}
		return true;
	}

} //namespace d2d
