
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ut/TestToolWnd.h"
#include "ut/UnitTest.h"
#include "GpUtilities.h"
#include "Image_fwd.h"
#include "ImagingDirect2D.h"
#include "ImagingWic.h"
#include "WicDibSection.h"
#include "ScopedGdi.h"
#include "UtilitiesEx.h"

#define new DEBUG_NEW


namespace ut
{
	bool Blit( CDC* pDC, const CRect& destRect, CDC* pSrcDC, const CRect& srcRect )
	{
		return pDC->StretchBlt( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height(), SRCCOPY ) != FALSE;
	}

	bool AlphaBlend( CDC* pDC, const CRect& destRect, CDC* pSrcDC, const CRect& srcRect, BYTE srcAlpha = 255, BYTE blendOp = AC_SRC_OVER )
	{
		BLENDFUNCTION blendFunc;
		blendFunc.BlendOp = blendOp;
		blendFunc.BlendFlags = 0;
		blendFunc.SourceConstantAlpha = srcAlpha;
		blendFunc.AlphaFormat = AC_SRC_ALPHA;

		return pDC->AlphaBlend( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height(), blendFunc ) != FALSE;
	}

	bool DrawBitmap( CDC* pDC, const CRect& destRect, CDC* pSrcDC, const CRect& srcRect )
	{
		return
			AlphaBlend( pDC, destRect, pSrcDC, srcRect ) ||
			Blit( pDC, destRect, pSrcDC, srcRect );
	}
}


namespace ut
{
	// CTestToolWnd implementation

	CTestToolWnd* CTestToolWnd::s_pWndTool = NULL;

	CTestToolWnd::CTestToolWnd( UINT elapseSelfDestroy )
		: CFrameWnd()
		, m_destroyTimer( this, DestroyTimerId, elapseSelfDestroy )
		, m_drawPos( CTestDevice::m_edgeSize )
	{
		ASSERT_NULL( s_pWndTool );
		s_pWndTool = this;
	}

	CTestToolWnd::~CTestToolWnd()
	{
		ASSERT( s_pWndTool == this );
		s_pWndTool = NULL;
	}

	CTestToolWnd* CTestToolWnd::AcquireWnd( UINT selfDestroySecs /*= 5*/ )
	{
		selfDestroySecs *= 1000;
		CTestToolWnd* pWndTool = s_pWndTool;
		if ( NULL == pWndTool )			// create first?
		{
			pWndTool = new CTestToolWnd( selfDestroySecs );
			CRect wndRect = MakeWindowRect();
			if ( !pWndTool->Create( GetClassName(), _T("TEST Tool"), WS_POPUPWINDOW | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE, wndRect, AfxGetMainWnd(), NULL, WS_EX_TOOLWINDOW ) )
			{
				ASSERT( false );
				delete pWndTool;
				return NULL;
			}
			ui::SetTopMost( pWndTool->m_hWnd );
		}

		ENSURE( pWndTool == s_pWndTool );

		if ( selfDestroySecs != 0 )
		{
			pWndTool->m_destroyTimer.SetElapsed( selfDestroySecs );
			pWndTool->m_destroyTimer.Start();
		}

		return pWndTool;
	}

	CRect CTestToolWnd::MakeWindowRect( void )
	{
		CRect desktopRect = ui::FindMonitorRect( AfxGetMainWnd()->m_hWnd, ui::Workspace );
		CRect wndRect( 0, 0, Width, Height );
		ui::AlignRect( wndRect, desktopRect, H_AlignRight | V_AlignTop );
		wndRect.OffsetRect( -20, 30 );
		return wndRect;
	}

	const TCHAR* CTestToolWnd::GetClassName( void )
	{
		static const CBrush fillBrush( color::html::LavenderBlush );
		static const TCHAR* pClassName = ::AfxRegisterWndClass( CS_SAVEBITS | CS_DBLCLKS, ::LoadCursor( NULL, IDC_ARROW ), fillBrush );
		return pClassName;
	}

	void CTestToolWnd::ResetDrawPos( void )
	{
		m_drawPos = CTestDevice::m_edgeSize;

		Invalidate();
		UpdateWindow();
	}


	// message handlers

	BEGIN_MESSAGE_MAP( CTestToolWnd, CFrameWnd )
		ON_WM_TIMER()
	END_MESSAGE_MAP()

	void CTestToolWnd::OnTimer( UINT_PTR eventId )
	{
		if ( m_destroyTimer.IsHit( eventId ) )
			DestroyWindow();
		else
			CFrameWnd::OnTimer( eventId );
	}


	// CTestDC class (private)

	class CTestDC : public CClientDC
	{
	public:
		CTestDC( CTestToolWnd* pToolWnd ) : CClientDC( pToolWnd ), m_scopedDrawText( this, pToolWnd, &GetCtrlFont() ), m_noBatching( 1 ) {}
	private:
		static CFont& GetCtrlFont( void );
	private:
		CScopedDrawText m_scopedDrawText;
		CScopedGdiBatchLimit m_noBatching;
	};


	CFont& CTestDC::GetCtrlFont( void )
	{
		static CFont font;
		if ( NULL == font.GetSafeHandle() )
			ui::MakeStandardControlFont( font, ui::CFontInfo() );
		return font;
	}


	// CTestDevice implementation

	const CSize CTestDevice::m_edgeSize( 10, 10 );

	CTestDevice::CTestDevice( CTestToolWnd* pToolWnd, TileAlign tileAlign /*= TileDown*/ )
		: m_tileAlign( tileAlign )
		, m_stripRect( 0, 0, 0, 0 )
	{
		Construct( pToolWnd );
	}

	CTestDevice::CTestDevice( UINT selfDestroySecs, TileAlign tileAlign /*= TileDown*/ )
		: m_tileAlign( tileAlign )
		, m_stripRect( 0, 0, 0, 0 )
	{
		Construct( selfDestroySecs != 0 ? CTestToolWnd::AcquireWnd( selfDestroySecs ) : NULL );
	}

	CTestDevice::~CTestDevice()
	{
		GotoNextStrip();		// advance to next strip to cascade succeeding tests
	}

	void CTestDevice::Construct( CTestToolWnd* pToolWnd )
	{
		m_pToolWnd = pToolWnd;
		if ( m_pToolWnd != NULL )
		{
			m_pTestDC.reset( new CTestDC( m_pToolWnd ) );
			m_stripRect.TopLeft() = m_stripRect.BottomRight() = m_pToolWnd->m_drawPos;
			m_pToolWnd->GetClientRect( &m_workAreaRect );
			m_workAreaRect.DeflateRect( m_edgeSize );
		}
	}

	void CTestDevice::GotoOrigin( void )
	{
		if ( IsEnabled() )
			m_pToolWnd->ResetDrawPos();
	}

	// tileSize is the size of previously drawn area
	//
	bool CTestDevice::GotoNextTile( void )
	{
		if ( !IsEnabled() )
			return false;

		switch ( m_tileAlign )
		{
			case TileRight:
				m_pToolWnd->m_drawPos.x += m_tileRect.Width() + m_edgeSize.cx;
				break;
			case TileDown:
				m_pToolWnd->m_drawPos.y += m_tileRect.Height() + m_edgeSize.cy;
				break;
		}
		if ( !m_workAreaRect.PtInRect( m_pToolWnd->m_drawPos ) )				// right pos overflows?
			return GotoNextStrip();
		return true;
	}

	bool CTestDevice::CascadeNextTile( void )
	{
		if ( !IsEnabled() )
			return false;
		if ( !GotoNextTile() )
			return false;

		// check if the subsequent tile would overflow partially (assuming same size)
		switch ( m_tileAlign )
		{
			case TileRight:
				if ( !m_workAreaRect.PtInRect( CPoint( m_pToolWnd->m_drawPos.x + m_tileRect.Width(), m_pToolWnd->m_drawPos.y ) ) )		// right pos overflows?
					return GotoNextTile();
				break;
			case TileDown:
				if ( !m_workAreaRect.PtInRect( CPoint( m_pToolWnd->m_drawPos.y + m_tileRect.Height(), m_pToolWnd->m_drawPos.x ) ) )		// bottom pos overflows?
					return GotoNextTile();
				break;
		}
		return true;
	}

	bool CTestDevice::GotoNextStrip( void )
	{
		if ( !IsEnabled() )
			return false;
		switch ( m_tileAlign )
		{
			case TileRight:
				m_pToolWnd->m_drawPos.x = m_edgeSize.cx;
				m_pToolWnd->m_drawPos.y += m_stripRect.Height() + m_edgeSize.cy;
				break;
			case TileDown:
				m_pToolWnd->m_drawPos.y = m_edgeSize.cy;
				m_pToolWnd->m_drawPos.x += m_stripRect.Width() + m_edgeSize.cx;
				break;
			default: ASSERT( false );
		}
		m_stripRect.TopLeft() = m_stripRect.BottomRight() = m_pToolWnd->m_drawPos;
		return m_workAreaRect.PtInRect( m_pToolWnd->m_drawPos ) != FALSE;		// false if out of visible area: done
	}

	void CTestDevice::StoreTileRect( const CRect& tileRect )
	{
		if ( !IsEnabled() )
			return;

		m_tileRect = tileRect;
		m_stripRect.BottomRight() = ui::MaxPoint( m_tileRect.BottomRight(), m_stripRect.BottomRight() );

		if ( !m_workAreaRect.PtInRect( m_tileRect.TopLeft() ) || !m_workAreaRect.PtInRect( m_tileRect.BottomRight() ) )		// overflows work area?
		{
			Region overflowRgn( gp::ToRect( m_tileRect ) );
			overflowRgn.Exclude( gp::ToRect( m_workAreaRect ) );

			Graphics graphics( *GetDC() );
			HatchBrush brush( HatchStyleLargeCheckerBoard, gp::MakeTransparentColor( color::Red, 70 ), gp::MakeTransparentColor( color::Orange, 80 ) );
			graphics.FillRegion( &brush, &overflowRgn );
		}
	}

	void CTestDevice::DrawTileFrame( COLORREF frameColor /*= color::LightGrey*/, int outerEdge /*= 1*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect frameRect = m_tileRect;
		frameRect.InflateRect( outerEdge, outerEdge );

		gp::FrameRect( GetDC(), frameRect, frameColor, 50 );
	}

	void CTestDevice::DrawTileFrame( const CRect& tileRect, COLORREF frameColor /*= color::LightGrey*/, int outerEdge /*= 1*/ )
	{
		StoreTileRect( tileRect );
		DrawTileFrame( frameColor, outerEdge );
	}

	void CTestDevice::DrawBitmap( HBITMAP hBitmap )
	{
		DrawBitmap( hBitmap, CBitmapInfo( hBitmap ).GetBitmapSize() );
	}

	void CTestDevice::DrawBitmap( HBITMAP hBitmap, const CSize& boundsSize, CDC* pSrcDC /*= NULL*/ )
	{
		if ( !IsEnabled() )
			return;

		CDC* pDC = GetDC();
		CDC memDC;
		if ( NULL == pSrcDC && memDC.CreateCompatibleDC( pDC ) )
			pSrcDC = &memDC;

		CSize bmpSize = CBitmapInfo( hBitmap ).GetBitmapSize();
		CRect boundsRect( m_pToolWnd->m_drawPos, boundsSize != CSize( 0, 0 ) ? boundsSize : bmpSize );

		CSize scaledBmpSize = ui::StretchToFit( boundsSize, bmpSize );

		CRect imageRect( boundsRect.TopLeft(), scaledBmpSize );
		ui::CenterRect( imageRect, boundsRect );

		int oldStretchMode = pDC->SetStretchBltMode( COLORONCOLOR );

		if ( pSrcDC != NULL )
		{
			CScopedGdiObj scopedBitmap( pSrcDC, hBitmap );
			ut::DrawBitmap( pDC, imageRect, pSrcDC, CRect( 0, 0, bmpSize.cx, bmpSize.cy ) );
		}
		pDC->SetStretchBltMode( oldStretchMode );

		DrawTileFrame( boundsRect, GetBitmapFrameColor( bmpSize, scaledBmpSize ) );
	}

	void CTestDevice::DrawBitmap( IWICBitmapSource* pWicBitmap, const CSize& boundsSize )
	{
		CWicDibSection dibSection( pWicBitmap );
		DrawBitmap( dibSection, boundsSize );
	}

	void CTestDevice::DrawBitmap( d2d::CRenderTarget* pBitmapRT, const d2d::CDrawBitmapTraits& traits, const CSize& boundsSize )
	{
		if ( !IsEnabled() )
			return;

		CRect boundsRect( m_pToolWnd->m_drawPos, boundsSize );

		CSize bmpSize = wic::GetBitmapSize( pBitmapRT->GetWicBitmap() );
		CSize scaledBmpSize = ui::StretchToFit( boundsSize, bmpSize );

		CRect imageRect( boundsRect.TopLeft(), scaledBmpSize );
		ui::CenterRect( imageRect, boundsRect );

		d2d::CDrawBitmapTraits dupTraits = traits;
		dupTraits.m_frameColor = GetBitmapFrameColor( bmpSize, scaledBmpSize );

		pBitmapRT->DrawBitmap( dupTraits, imageRect );
		StoreTileRect( boundsRect );
	}

	void CTestDevice::DrawIcon( HICON hIcon, const CSize& boundsSize, UINT flags /*= DI_NORMAL*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect iconRect = CRect( m_pToolWnd->m_drawPos, boundsSize );

		if ( !::DrawIconEx( GetDC()->GetSafeHdc(), iconRect.left, iconRect.top, hIcon, iconRect.Width(), iconRect.Height(), 0, NULL, flags ) )
			ASSERT( false );

		DrawTileFrame( iconRect );
	}

	void CTestDevice::DrawImage( CImageList* pImageList, int index, UINT style /*= ILD_TRANSPARENT*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect imageRect( m_pToolWnd->m_drawPos, gdi::GetImageSize( *pImageList ) );

		pImageList->Draw( GetDC(), index, imageRect.TopLeft(), style );
		DrawTileFrame( imageRect );
	}

	void CTestDevice::DrawImageList( CImageList* pImageList, bool putTags /*= false*/, UINT style /*= ILD_TRANSPARENT*/ )
	{
		if ( !IsEnabled() )
			return;

		ASSERT_PTR( pImageList );

		UINT imageCount = pImageList->GetImageCount();
		CSize imageSize = gdi::GetImageSize( *pImageList );

		CRect imageRect = CRect( m_pToolWnd->m_drawPos, imageSize );
		CRect boundsRect = imageRect;			// bounds drawn

		for ( UINT i = 0; i != imageCount; ++i, imageRect.OffsetRect( 0, imageSize.cy ) )
		{
			pImageList->Draw( GetDC(), i, imageRect.TopLeft(), style );
			boundsRect |= imageRect;

			if ( putTags )
			{
				std::tstring tag = str::Format( _T("[%d]"), i );
				int width = ui::GetTextSize( GetDC(), tag.c_str() ).cx + 5;

				CRect textRect = imageRect;
				textRect.left = imageRect.right + 5;
				textRect.right = textRect.left + width;
				GetDC()->DrawText( tag.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

				boundsRect |= textRect;
			}
		}
		DrawTileFrame( boundsRect );
	}

	void CTestDevice::DrawTextInfo( const std::tstring& text )
	{
		if ( !IsEnabled() )
			return;
		if ( text.empty() )
			return;

		enum { TextEdge = 3 };
		CRect textRect( CPoint( 0, 0 ), ui::GetTextSize( GetDC(), text.c_str(), DT_EDITCONTROL ) );

		CRect boundsRect( m_pToolWnd->m_drawPos, textRect.Size() + CSize( TextEdge * 2, TextEdge * 2 ) );
		ui::CenterRect( textRect, boundsRect );
		GetDC()->DrawText( text.c_str(), -1, &textRect, DT_EDITCONTROL );
		DrawTileFrame( boundsRect );
	}

	void CTestDevice::DrawBitmapInfo( HBITMAP hBitmap )
	{
		DrawTextInfo( CBitmapInfo( hBitmap ).FormatDbg() );
	}

	COLORREF CTestDevice::GetBitmapFrameColor( const CSize& bmpSize, const CSize& scaledBmpSize )
	{
		if ( scaledBmpSize.cx < bmpSize.cx )			// shrunk
			return color::Red;
		else if ( scaledBmpSize.cx > bmpSize.cx )		// stretched
			return color::LightBlue;
		return color::LightGrey;
	}

} //namespace ut


#endif //_DEBUG
