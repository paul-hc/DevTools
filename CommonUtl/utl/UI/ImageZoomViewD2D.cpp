
#include "stdafx.h"
#include "ImageZoomViewD2D.h"
#include "ImagingWic.h"
#include "DirectWrite.h"
#include "AnimatedFrameComposer.h"
#include "RenderingDirect2D.h"
#include "ImageInfoGadget.h"
#include "GdiCoords.h"
#include "WicAnimatedImage.h"
#include "WicDibSection.h"			// for printing
#include "StringUtilities.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CImageRenderTarget implementation

	CImageRenderTarget::CImageRenderTarget( ui::IImageZoomView* pImageView )
		: CWindowRenderTarget( pImageView->GetZoomView()->GetScrollView() )
		, m_pImageView( pImageView )
		, m_accentFrameColor( ::GetSysColor( COLOR_ACTIVECAPTION ) )	// COLOR_HIGHLIGHT
		, m_animTimer( m_pImageView->GetZoomView()->GetScrollView(), AnimateTimer, 1000 )
		, m_pImageInfoGadget( new d2d::CImageInfoGadget( m_pImageView, dw::CreateTextFormat( L"Arial" /*L"Calibri"*/, 10, DWRITE_FONT_WEIGHT_BOLD ) ) )
		, m_pAccentFrameGadget( MakeAccentFrameGadget() )
	{
		ASSERT_PTR( m_pImageView->GetZoomView()->GetScrollView()->GetSafeHwnd() );

		AddGadget( m_pImageInfoGadget.get() );
		AddGadget( m_pAccentFrameGadget.get() );
	}

	CImageRenderTarget::~CImageRenderTarget()
	{
	}

	CFrameGadget* CImageRenderTarget::MakeAccentFrameGadget( void ) const
	{
		const D2D1_COLOR_F colors[] = { ToColor( m_accentFrameColor, 70 ), ToColor( color::White, 20 ) };
		CFrameGadget* pAccentFrameGadget = new CFrameGadget( 3, ARRAY_PAIR( colors ) );

		pAccentFrameGadget->SetFrameStyle( GradientFrameRadialCorners );

	#if 0
		// debugging: use high contrast colours
		static const D2D1_COLOR_F s_debugColors[] = { ToColor( color::Red, 90 ), ToColor( color::Yellow, 80 ), ToColor( color::BrightGreen, 70 ) };
		pAccentFrameGadget->SetColors( ARRAY_PAIR( s_debugColors ) );
		pAccentFrameGadget->SetFrameSize( 10 );
		pAccentFrameGadget->SetFrameStyle( OutlineGradientFrame );
	#endif

		return pAccentFrameGadget;
	}

	CWicImage* CImageRenderTarget::GetImage( void ) const
	{
		return m_pImageView->GetImage();
	}

	void CImageRenderTarget::HandleAnimEvent( void )
	{
		if ( m_pAnimComposer.get() != NULL && m_pAnimComposer->UsesImage( GetImage() ) )
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

		m_pImageInfoGadget->BuildInfo();
	}


	void CImageRenderTarget::DiscardDeviceResources( void )
	{
		__super::DiscardDeviceResources();

		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->Reset();
	}

	bool CImageRenderTarget::CreateDeviceResources( void )
	{
		if ( !__super::CreateDeviceResources() )
			return false;

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

	bool CImageRenderTarget::IsGadgetVisible( const IGadgetComponent* pGadget ) const
	{
		const ui::IZoomView* pZoomView = m_pImageView->GetZoomView();

		if ( NULL == GetImage() || pZoomView->HasViewStatusFlag( ui::ZoomMouseTracking ) )			// don't show any gadget in zoom tracking mode
			return false;

		if ( pGadget == m_pAccentFrameGadget.get() )
			if ( pZoomView->HasViewStatusFlag( ui::FullScreen ) )				// we use background highlighting in full-screen mode
				return false;
			else if ( !pZoomView->IsAccented() )
				return false;

		return __super::IsGadgetVisible( pGadget );
	}

	void CImageRenderTarget::DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords )
	{
		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->DrawBitmap( coords, bmpCoords );			// draw current animated frame
		else
			__super::DrawBitmap( coords, bmpCoords );					// draw static image
	}

	void CImageRenderTarget::PreDraw( const CViewCoords& coords )
	{
		coords;
		const ui::IZoomView* pZoomView = m_pImageView->GetZoomView();
		COLORREF bkColor = pZoomView->GetBkColor();

		if ( pZoomView->IsAccented() )
			if ( pZoomView->HasViewStatusFlag( ui::FullScreen | ui::ZoomMouseTracking ) || NULL == GetImage() )
				bkColor = CBaseZoomView::MakeAccentedBkColor( bkColor );					// use accented background highlight

		ClearBackground( bkColor );

		__super::PreDraw( coords );			// erase the gadgets
	}


	bool CImageRenderTarget::DrawImage( const CViewCoords& coords, const CBitmapCoords& bmpCoords )
	{
		if ( !CanRender() )
			return false;				// window is occluded

		SetupCurrentImage();

		return RenderDone == Render( coords, bmpCoords );
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

ui::IZoomView* CImageZoomViewD2D::GetZoomView( void )
{
	return this;
}

CWicImage* CImageZoomViewD2D::QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const
{
	CWicImage* pImage = GetImage();

	rFileDetails.Reset( pImage );
	return pImage;						// may be reused by overrides
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
		m_pImageRT->EnsureDeviceResources();

		if ( m_pImageRT->IsValidTarget() )
		{
			CPoint scrollPos = GetScrollPosition();

			m_drawTraits.SetScrollPos( scrollPos );													// apply translation transform according to view's scroll position
			m_drawTraits.SetAutoInterpolationMode( GetContentRect().Size(), GetSourceSize() );		// force smooth mode when shrinking bitmap

			d2d::CViewCoords viewCoords( _GetClientRect(), GetContentRect() );
			d2d::CBitmapCoords bmpCoords( m_drawTraits );

			m_pImageRT->DrawImage( viewCoords, bmpCoords );
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
	m_pImageRT->CreateDeviceResources();
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
	CSize screenResolution( (DWORD)lParam ); screenResolution;		// just for illustration

	Invalidate();

#if (_MFC_VER <= 0x0900)		// Microsoft Foundation Classes version 9.00 (up to Visual Studio 2008)
	return __super::OnDisplayChange( bitsPerPixel, lParam );
#else							// e.g. newer _MFC_VER 0x0C00 (Visual Studio 2013)
	__super::OnDisplayChange( bitsPerPixel, screenResolution.cx, screenResolution.cy );
	return Default();
#endif
}
