
#include "pch.h"
#include "ImagingDirect2D.h"
#include "ImagingWic.h"
#include "GdiCoords.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CDrawBitmapTraits implementation

	CDrawBitmapTraits::CDrawBitmapTraits( COLORREF bkColor /*= CLR_NONE*/, UINT opacityPct /*= 100*/ )
		: m_smoothingMode( utl::Default )
		, m_bkColor( bkColor )
		, m_opacity( static_cast<float>( opacityPct ) / 100.f )
		, m_transform( D2D1::Matrix3x2F::Identity() )
		, m_frameColor( CLR_NONE )
	{
	}

	void CDrawBitmapTraits::SetScrollPos( const POINT& scrollPos )
	{
		// apply translation transform according to view's scroll position
		m_transform = D2D1::Matrix3x2F::Translation( (float)-scrollPos.x, (float)-scrollPos.y );
	}

	void CDrawBitmapTraits::Draw( ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap* pBitmap, const CRect& destRect, const CRect* pSrcRect /*= nullptr*/ ) const
	{
		// caller should initiate BeginDraw
		ASSERT_PTR( pRenderTarget );

		pRenderTarget->SetTransform( &m_transform );

		D2D_RECT_F destRectF = d2d::ToRectF( destRect );
		if ( m_bkColor != CLR_NONE )
		{
			CComPtr<ID2D1SolidColorBrush> pBrush;
			if ( HR_OK( pRenderTarget->CreateSolidColorBrush( ToColor( m_bkColor ), &pBrush ) ) )
				pRenderTarget->FillRectangle( &destRectF, pBrush );				// clear the image background (not the entire client rect)
		}

		if ( pBitmap != nullptr )
		{
			CRect bmpRect = pSrcRect != nullptr ? *pSrcRect : CRect( CPoint( 0, 0 ), FromSizeU( pBitmap->GetPixelSize() ) );
			D2D_RECT_F srcRect = d2d::ToRectF( bmpRect );
			D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = ui::FitsInside( destRect.Size(), bmpRect.Size() )		// enlarge scaling?
				? CSharedTraits::ToInterpolationMode( IsSmoothingMode() )		// use current enlarge scaling mode
				: D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;						// shrinking: always use smooting (dithering)

			pRenderTarget->DrawBitmap( pBitmap, destRectF, m_opacity, interpolationMode, pSrcRect != nullptr ? &srcRect : nullptr );
		}

		if ( m_frameColor != CLR_NONE )
		{
			CComPtr<ID2D1SolidColorBrush> pBrush;
			if ( HR_OK( pRenderTarget->CreateSolidColorBrush( ToColor( m_frameColor ), &pBrush ) ) )
				pRenderTarget->DrawRectangle( &destRectF, pBrush );				// draw a frame around the image
		}
	}


	// CRenderTarget implementation

	bool CRenderTarget::CanRender( void ) const
	{
		return IsValidTarget();
	}

	void CRenderTarget::AddGadget( IGadgetComponent* pGadget )
	{
		ASSERT_PTR( pGadget );
		utl::PushUnique( m_gadgets, pGadget );
		checked_static_cast<CGadgetBase*>( pGadget )->SetRenderHost( this );
	}

	bool CRenderTarget::IsGadgetVisible( const IGadgetComponent* pGadget ) const
	{
		ASSERT_PTR( pGadget );
		return pGadget->IsValid();
	}

	void CRenderTarget::DiscardDeviceResources( void )
	{
		utl::for_each( m_gadgets, std::mem_fn( &IGadgetComponent::DiscardDeviceResources ) );
	}

	bool CRenderTarget::CreateDeviceResources( void )
	{
		utl::for_each( m_gadgets, std::mem_fn( &IGadgetComponent::CreateDeviceResources ) );
		return true;
	}


	bool CRenderTarget::SetWicBitmap( IWICBitmapSource* pWicBitmap )
	{
		if ( pWicBitmap == m_pWicBitmap )
			return false;				// same bitmap as the old one

		m_pWicBitmap = pWicBitmap;
		ReleaseBitmap();				// release previous D2D bitmap
		return true;
	}

	ID2D1Bitmap* CRenderTarget::GetBitmap( void )
	{
		if ( m_pWicBitmap != nullptr && nullptr == m_pBitmap )
			if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
				HR_AUDIT( pRenderTarget->CreateBitmapFromWicBitmap( m_pWicBitmap, nullptr, &m_pBitmap ) );

		return m_pBitmap;
	}

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

				DiscardDeviceResources();

				if ( CWnd* pWnd = GetWindow() )
					pWnd->Invalidate( TRUE );			// device loss: force a re-render
			}
	}

	void CRenderTarget::ClearBackground( COLORREF bkColor )
	{
		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
			pRenderTarget->Clear( ToColor( bkColor ) );
	}

	RenderResult CRenderTarget::Render( const CViewCoords& coords, const CBitmapCoords& bmpCoords )
	{
		EnsureDeviceResources();		// lazy render target (if not yet created or device lost)

		if ( !CanRender() )
			return RenderError;			// window is occluded

		TScopedDraw scopedDraw( this );

		PreDraw( coords );
		DrawBitmap( coords, bmpCoords );
		PostDraw( coords );

		if ( !IsValidTarget() )			// resources discarded?
			return DeviceLoss;			// device was lost

		return RenderDone;
	}

	void CRenderTarget::DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords )
	{
		bmpCoords.m_dbmTraits.Draw( GetRenderTarget(), GetBitmap(), coords.m_contentRect, bmpCoords.m_pSrcBmpRect );
	}

	void CRenderTarget::PreDraw( const CViewCoords& coords )
	{
		for ( std::vector<IGadgetComponent*>::const_iterator itGadget = m_gadgets.begin(); itGadget != m_gadgets.end(); ++itGadget )
			( *itGadget )->EraseBackground( coords );
	}

	void CRenderTarget::PostDraw( const CViewCoords& coords )
	{
		for ( std::vector<IGadgetComponent*>::const_iterator itGadget = m_gadgets.begin(); itGadget != m_gadgets.end(); ++itGadget )
			if ( IsGadgetVisible( *itGadget ) )
				( *itGadget )->Draw( coords );
	}


	// CWindowRenderTarget implementation

	bool CWindowRenderTarget::Resize( const SIZE& clientSize )
	{
		if ( !HR_OK( m_pWndRenderTarget->Resize( ToSizeU( clientSize ) ) ) )			// IMP: if couldn't resize, release the device and we'll recreate it during the next render pass
		{
			DiscardDeviceResources();
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

	void CWindowRenderTarget::DiscardDeviceResources( void )
	{
		__super::DiscardDeviceResources();

		m_pWndRenderTarget = nullptr;
		ReleaseBitmap();
	}

	bool CWindowRenderTarget::CreateDeviceResources( void )
	{
		ASSERT_NULL( m_pWndRenderTarget );

		CRect clientRect;
		m_pWnd->GetClientRect( &clientRect );

		D2D1_RENDER_TARGET_PROPERTIES rtProperties = D2D1::RenderTargetProperties();

		// use the screen DPI to allow direct mapping between image pixels and desktop pixels in different system DPI settings
		CFactory::Factory()->GetDesktopDpi( &rtProperties.dpiX, &rtProperties.dpiY );		// usually 96 DPI

		if ( !HR_OK( CFactory::Factory()->CreateHwndRenderTarget( rtProperties,
																  D2D1::HwndRenderTargetProperties( m_pWnd->GetSafeHwnd(), ToSizeU( clientRect.Size() ) ),
																  &m_pWndRenderTarget ) ) )
			return false;

		return __super::CreateDeviceResources();
	}

	bool CWindowRenderTarget::CanRender( void ) const
	{
		return __super::CanRender() && !HasFlag( m_pWndRenderTarget->CheckWindowState(), D2D1_WINDOW_STATE_OCCLUDED );
	}


	// CDCRenderTarget implementation

	void CDCRenderTarget::DiscardDeviceResources( void )
	{
		m_pDcRenderTarget = nullptr;
		ReleaseBitmap();
	}

	bool CDCRenderTarget::CreateDeviceResources( void )
	{
		ASSERT_NULL( m_pDcRenderTarget );

		CRect clientRect;
		GetWindow()->GetClientRect( &clientRect );

		// use the screen DPI to allow direct mapping between image pixels and desktop pixels in different system DPI settings
		float dpiX, dpiY;
		CFactory::Factory()->GetDesktopDpi( &dpiX, &dpiY );			// usually 96 DPI

		D2D1_RENDER_TARGET_PROPERTIES rtProperties = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE ),			// required for GDI rendering
			dpiX, dpiY,
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_DEFAULT );

		if ( HR_OK( CFactory::Factory()->CreateDCRenderTarget( &rtProperties, &m_pDcRenderTarget ) ) )
			if ( Resize( clientRect ) )
				return true;

		return false;
	}

	bool CDCRenderTarget::Resize( const RECT& subRect )
	{
		ASSERT_PTR( m_pDC->GetSafeHdc() );

		EnsureDeviceResources();			// lazy render target (if not yet created or device lost)

		if ( !HR_OK( m_pDcRenderTarget->BindDC( m_pDC->GetSafeHdc(), &subRect ) ) )			// bind the DC to the render target
		{	// IMP: if couldn't resize, release the device and we'll recreate it during the next render pass
			DiscardDeviceResources();
			return false;
		}
		return true;
	}

} //namespace d2d
