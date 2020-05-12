
#include "stdafx.h"
#include "ImageZoomViewD2D.h"
#include "ImagingWic.h"
#include "DirectWrite.h"
#include "AnimatedFrameComposer.h"
#include "RenderingDirect2D.h"
#include "Color.h"
#include "GdiCoords.h"
#include "WicAnimatedImage.h"
#include "WicDibSection.h"			// for printing
#include "Utilities.h"
#include "StringUtilities.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	void CImageFileDetails::Reset( const CWicImage* pImage /*= NULL*/ )
	{
		m_filePath.Clear();
		m_isAnimated = false;
		m_framePos = m_frameCount = m_fileSize = m_navigPos = m_navigCount = 0;
		m_dimensions = CSize( 0, 0 );

		if ( pImage != NULL )
		{
			m_filePath = pImage->GetImagePath();
			m_isAnimated = pImage->IsAnimated();
			m_framePos = pImage->GetFramePos();
			m_frameCount = pImage->GetFrameCount();
			m_navigCount = 1;
		}
	}

	double CImageFileDetails::GetMegaPixels( void ) const
	{
		return num::ConvertFileSize( ui::GetSizeArea( m_dimensions ), num::MegaBytes ).first;
	}
}


namespace dw
{
	class CImageInfoText
	{
	public:
		CImageInfoText( CImageZoomViewD2D* pZoomView, IDWriteTextFormat* pInfoFont );
		~CImageInfoText() {}

		void BuildInfo( void );

		void DiscardResources( void );
		bool CreateResources( ID2D1RenderTarget* pRenderTarget );

		bool IsValid( void ) const { return m_info.IsValid() && m_pTextLayout != NULL; }
		bool MakeTextLayout( ID2D1RenderTarget* pRenderTarget );

		void DrawImageInfoText( ID2D1RenderTarget* pRenderTarget, const d2d::CViewCoords& coords );
	private:
		CImageZoomViewD2D* m_pZoomView;
		ui::CImageFileDetails m_info;

		// device-independent
		CComPtr< IDWriteTextFormat > m_pInfoFont;

		// text rendering resources
		CComPtr< ID2D1Brush > m_pBkBrush;
		CComPtr< ID2D1Brush > m_pTextBrush;
		CComPtr< ID2D1Brush > m_pDimensionsBrush;
		CComPtr< ID2D1Brush > m_pNavigBrush;

		CComPtr< IDWriteTextLayout > m_pTextLayout;
	};


	// CImageInfoText implementation

	CImageInfoText::CImageInfoText( CImageZoomViewD2D* pZoomView, IDWriteTextFormat* pInfoFont )
		: m_pZoomView( pZoomView )
		, m_pInfoFont( pInfoFont )
	{
		ASSERT_PTR( m_pZoomView );
		ASSERT_PTR( m_pInfoFont );
	}

	void CImageInfoText::DiscardResources( void )
	{
		m_pBkBrush = NULL;
		m_pTextBrush = NULL;
		m_pDimensionsBrush = NULL;
		m_pNavigBrush = NULL;

		m_pTextLayout = NULL;
	}

	bool CImageInfoText::CreateResources( ID2D1RenderTarget* pRenderTarget )
	{
		// text drawing objects
		enum { MyOliveGreen = RGB(204, 251, 93) };

		d2d::CreateAsSolidBrush( m_pBkBrush, pRenderTarget, d2d::ToColor( color::VeryDarkGrey, 60 ) );
		d2d::CreateAsSolidBrush( m_pTextBrush, pRenderTarget, d2d::ToColor( color::BrightGreen, 100 ) );
		d2d::CreateAsSolidBrush( m_pDimensionsBrush, pRenderTarget, d2d::ToColor( MyOliveGreen, 90 ) );
		d2d::CreateAsSolidBrush( m_pNavigBrush, pRenderTarget, d2d::ToColor( color::LightOrange ) );
		return true;
	}

	void CImageInfoText::BuildInfo( void )
	{
		m_pZoomView->QueryImageFileDetails( m_info );
		m_pTextLayout = NULL;
	}

	bool CImageInfoText::MakeTextLayout( ID2D1RenderTarget* pRenderTarget )
	{
		ASSERT_PTR( pRenderTarget );

		enum InfoField { FileName, Dimensions, NavigCounts, ZoomPercent,  _InfoFieldCount };

		dw::CTextLayout textLayout( _InfoFieldCount, _T(" ") );		// effect ranges indexed by InfoField

		textLayout.AddField( m_info.m_filePath.GetFilename() );

		textLayout.AddField( str::Format( _T("(%d x %d = %.2f MP, %s)"),
			m_info.m_dimensions.cx, m_info.m_dimensions.cy,
			m_info.GetMegaPixels(),
			num::FormatFileSize( m_info.m_fileSize ).c_str()
		) );

		textLayout.AddField( m_info.HasNavigInfo() ? str::Format( _T("[ %d / %d ]"), m_info.m_navigPos + 1, m_info.m_navigCount ) : str::GetEmpty() );
		textLayout.AddField( str::Format( _T("%d %%"), m_pZoomView->GetZoomPct() ) );

		D2D_SIZE_F boundsSize = d2d::ToSizeF( ui::GetScreenSize() );		// start with the entire screen

		m_pTextLayout = NULL;
		if ( !HR_OK( dw::CTextFactory::Factory()->CreateTextLayout( textLayout.GetFullText().c_str(), textLayout.GetFullTextLength(), m_pInfoFont, boundsSize.width, boundsSize.height, &m_pTextLayout ) ) )
			return false;

		// apply text effects
		HR_VERIFY( m_pTextLayout->SetDrawingEffect( m_pDimensionsBrush, textLayout.GetFieldRangeAt( Dimensions ) ) );
		HR_VERIFY( m_pTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_NORMAL, textLayout.GetFieldRangeAt( Dimensions ) ) );

		HR_VERIFY( m_pTextLayout->SetDrawingEffect( m_pNavigBrush, textLayout.GetFieldRangeAt( NavigCounts ) ) );

		HR_VERIFY( m_pTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_NORMAL, textLayout.GetFieldRangeAt( ZoomPercent ) ) );
		//HR_VERIFY( m_pTextLayout->SetFontStyle( DWRITE_FONT_STYLE_ITALIC, textLayout.GetFieldRangeAt( ZoomPercent ) ) );
		return true;
	}

	void CImageInfoText::DrawImageInfoText( ID2D1RenderTarget* pRenderTarget, const d2d::CViewCoords& coords )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( m_pTextLayout );

		pRenderTarget->SetTransform( D2D1::IdentityMatrix() );

		enum Metrics { EdgeH = 6, EdgeV = 3 };

		CSize textSize = d2d::FromSizeF( dw::ComputeTextSize( m_pTextLayout ) );

		CRect textRect = coords.m_clientRect;
		textRect.DeflateRect( EdgeH, EdgeV );
		textRect.top = textRect.bottom - textSize.cy;
		textRect.right = textRect.left + textSize.cx;

		CRect backgroundRect = textRect;
		backgroundRect.InflateRect( 3, 1, 2, 2 );

		static const float s_roundRadius = 3.0f;
		D2D1_ROUNDED_RECT bkRoundRectF = { d2d::ToRectF( backgroundRect ), s_roundRadius, s_roundRadius };
		pRenderTarget->FillRoundedRectangle( bkRoundRectF, m_pBkBrush );

		D2D1_POINT_2F textOrigin = d2d::ToPointF( textRect.TopLeft() );
		pRenderTarget->DrawTextLayout( textOrigin, m_pTextLayout, m_pTextBrush );
	}
}


namespace d2d
{
	// CImageRenderTarget implementation

	CImageRenderTarget::CImageRenderTarget( CImageZoomViewD2D* pZoomView )
		: CWindowRenderTarget( pZoomView )
		, m_pZoomView( pZoomView )
		, m_accentFrameColor( ::GetSysColor( COLOR_ACTIVECAPTION ) )	//COLOR_HIGHLIGHT
		, m_animTimer( m_pZoomView, AnimateTimer, 1000 )
		, m_pImageInfoText( new dw::CImageInfoText( m_pZoomView, dw::CreateTextFormat( L"Arial" /*L"Calibri"*/, 10, DWRITE_FONT_WEIGHT_BOLD ) ) )
	{
		ASSERT_PTR( m_pZoomView->GetSafeHwnd() );

		const D2D1_COLOR_F colors[] = { ToColor( m_accentFrameColor, 70 ), ToColor( color::White, 20 ) };
		m_pAccentFrame.reset( new CFrameFacet( 3, ARRAY_PAIR( colors ) ) );
		m_pAccentFrame->SetFrameStyle( GradientFrameRadialCorners );

	#if 0
		// debugging: use high contrast colours
		static const D2D1_COLOR_F s_debugColors[] = { ToColor( color::Red, 90 ), ToColor( color::Yellow, 80 ), ToColor( color::BrightGreen, 70 ) };
		m_pAccentFrame->SetColors( ARRAY_PAIR( s_debugColors ) );
		m_pAccentFrame->SetFrameSize( 10 );
		m_pAccentFrame->SetFrameStyle( OutlineGradientFrame );
	#endif
	}

	CImageRenderTarget::~CImageRenderTarget()
	{
	}

	void CImageRenderTarget::SetAccentFrameColor( COLORREF accentFrameColor )
	{
		m_accentFrameColor = accentFrameColor;
		m_pAccentFrame->DiscardResources();
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

		m_pImageInfoText->BuildInfo();

		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
			m_pImageInfoText->MakeTextLayout( pRenderTarget );
	}


	void CImageRenderTarget::DiscardResources( void )
	{
		CWindowRenderTarget::DiscardResources();

		if ( m_pAnimComposer.get() != NULL )
			m_pAnimComposer->Reset();

		m_pImageInfoText->DiscardResources();
		m_pAccentFrame->DiscardResources();
	}

	bool CImageRenderTarget::CreateResources( void )
	{
		if ( !CWindowRenderTarget::CreateResources() )
			return false;

		if ( ID2D1RenderTarget* pRenderTarget = GetRenderTarget() )
		{
			m_pImageInfoText->CreateResources( pRenderTarget );

			if ( m_accentFrameColor != CLR_NONE )
				m_pAccentFrame->CreateResources( pRenderTarget );
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
		COLORREF bkColor = m_pZoomView->GetBkColor();

		if ( m_pZoomView->IsAccented() )
			if ( m_pZoomView->HasViewStatusFlag( CBaseZoomView::FullScreen | CBaseZoomView::ZoomMouseTracking ) )
				bkColor = CBaseZoomView::MakeAccentedBkColor( bkColor );					// use accented background highlight

		ClearBackground( bkColor );
	}

	void CImageRenderTarget::PostDraw( const CViewCoords& coords )
	{
		enum { FrameSize = 50 };

		ID2D1RenderTarget* pRenderTarget = GetRenderTarget();

		if ( !m_pZoomView->HasViewStatusFlag( CBaseZoomView::ZoomMouseTracking ) )			// don't show the text info in zoom tracking mode
			if ( m_pImageInfoText.get() != NULL )
				if ( m_pImageInfoText->IsValid() )
					m_pImageInfoText->DrawImageInfoText( pRenderTarget, coords );

		if ( !m_pZoomView->HasViewStatusFlag( CBaseZoomView::FullScreen | CBaseZoomView::ZoomMouseTracking ) )		// don't show the frame in zoom tracking mode
			if ( m_pZoomView->IsAccented() )
				m_pAccentFrame->Draw( pRenderTarget, coords.m_contentRect );
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

			m_drawTraits.SetScrollPos( scrollPos );												// apply translation transform according to view's scroll position
			m_drawTraits.SetAutoInterpolationMode( GetContentRect().Size(), GetSourceSize() );	// force smooth mode when shrinking bitmap

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
	CSize screenResolution( (DWORD)lParam ); screenResolution;		// just for illustration

	Invalidate();

#if (_MFC_VER <= 0x0900)		// Microsoft Foundation Classes version 9.00 (up to Visual Studio 2008)
	return __super::OnDisplayChange( bitsPerPixel, lParam );
#else							// e.g. newer _MFC_VER 0x0C00 (Visual Studio 2013)
	__super::OnDisplayChange( bitsPerPixel, screenResolution.cx, screenResolution.cy );
	return Default();
#endif
}
