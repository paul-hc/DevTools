
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/TestToolWnd.h"
#include "test/UnitTest.h"
#include "GpUtilities.h"
#include "Image_fwd.h"
#include "ImagingDirect2D.h"
#include "ImagingWic.h"
#include "ImageCommandLookup.h"
#include "WicDibSection.h"
#include "Icon.h"
#include "IconGroup.h"
#include "ScopedGdi.h"
#include "WndUtilsEx.h"
#include <afxtoolbarimages.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	// CTestDC class (private)

	class CTestDC : public CClientDC
	{
	public:
		CTestDC( CTestToolWnd* pToolWnd )
			: CClientDC( pToolWnd )
			, m_scopedDrawText( this, pToolWnd, &GetCtrlFont() )
			, m_noBatching( 1 )
		{
		}
	private:
		static CFont& GetCtrlFont( void );
	private:
		CScopedDrawText m_scopedDrawText;
		CScopedGdiBatchLimit m_noBatching;
	};

	CFont& CTestDC::GetCtrlFont( void )
	{
		static CFont font;
		if ( nullptr == font.GetSafeHandle() )
			ui::MakeStandardControlFont( font, ui::CFontInfo() );
		return font;
	}


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

	void DrawGlyphFramePoints( CDC* pDC, const CRect& imageRect )
	{
		CRect frameBounds = imageRect;

		frameBounds.InflateRect( 1, 1 );
		pDC->SetPixel( frameBounds.left, frameBounds.top, color::Blue );
		pDC->SetPixel( frameBounds.left, frameBounds.bottom, color::Blue );
		pDC->SetPixel( frameBounds.right, frameBounds.top, color::Red );
		pDC->SetPixel( frameBounds.right, frameBounds.bottom, color::Red );
	}
}


namespace ut
{
	static const TCHAR s_titleBase[] = _T("UI TEST Tool");


	// CTestToolWnd implementation

	CTestToolWnd* CTestToolWnd::s_pWndTool = nullptr;

	CTestToolWnd::CTestToolWnd( UINT elapseSelfDestroy )
		: CFrameWnd()
		, m_destroyTimer( this, DestroyTimerId, elapseSelfDestroy )
		, m_disableEraseBk( false )
		, m_drawPos( CTestDevice::m_edgeSize )
	{
		ASSERT_NULL( s_pWndTool );
		s_pWndTool = this;

		ui::MakeStandardControlFont( m_headlineFont, ui::CFontInfo( ui::Bold, 130 ) );
	}

	CTestToolWnd::~CTestToolWnd()
	{
		ASSERT( s_pWndTool == this );
		s_pWndTool = nullptr;
	}

	CTestToolWnd* CTestToolWnd::AcquireWnd( UINT selfDestroySecs /*= 5*/ )
	{
		selfDestroySecs *= 1000;
		CTestToolWnd* pWndTool = s_pWndTool;
		if ( nullptr == pWndTool )			// create first?
		{
			pWndTool = new CTestToolWnd( selfDestroySecs );
			CRect wndRect = MakeWindowRect();
			if ( !pWndTool->Create( GetClassName(), s_titleBase, WS_POPUPWINDOW | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE, wndRect, AfxGetMainWnd(), nullptr, WS_EX_TOOLWINDOW ) )
			{
				ASSERT( false );
				delete pWndTool;
				return nullptr;
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

	void CTestToolWnd::DisableEraseBk( void )
	{
		// Hack: set to true to prevent erasing the background when a d2d::CDCRenderTarget goes out of scope - e.g. in CImagingD2DTests::Run(), and sends a WM_DWMNCRENDERINGCHANGED message.
		//	This was difficult to debug and required extra code for debugging and capturing the cause of the window erase.

		if ( s_pWndTool != nullptr )
			s_pWndTool->m_disableEraseBk = true;
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
		static const TCHAR* pClassName = ::AfxRegisterWndClass( CS_SAVEBITS | CS_DBLCLKS, ::LoadCursor( nullptr, IDC_ARROW ), fillBrush );
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
		ON_WM_ERASEBKGND()
	END_MESSAGE_MAP()

	void CTestToolWnd::OnTimer( UINT_PTR eventId )
	{
		if ( m_destroyTimer.IsHit( eventId ) )
			DestroyWindow();
		else
			CFrameWnd::OnTimer( eventId );
	}

	BOOL CTestToolWnd::OnEraseBkgnd( CDC* pDC )
	{
		if ( m_disableEraseBk )
			return TRUE;

		return __super::OnEraseBkgnd( pDC );
	}


	// CTestDevice implementation

	const CSize CTestDevice::m_edgeSize( 10, 10 );

	CTestDevice::CTestDevice( CTestToolWnd* pToolWnd, TileAlign tileAlign /*= ut::TileRight*/ )
		: m_tileAlign( tileAlign )
		, m_stripRect( 0, 0, 0, 0 )
		, m_scatterCaption( false )
	{
		Construct( pToolWnd );
	}

	CTestDevice::CTestDevice( UINT selfDestroySecs, TileAlign tileAlign /*= TileRight*/ )
		: m_tileAlign( tileAlign )
		, m_stripRect( 0, 0, 0, 0 )
		, m_scatterCaption( false )
	{
		Construct( selfDestroySecs != 0 ? CTestToolWnd::AcquireWnd( selfDestroySecs ) : nullptr );
	}

	CTestDevice::~CTestDevice()
	{
		GotoNextStrip();		// advance to next strip to cascade succeeding tests
	}

	void CTestDevice::Construct( CTestToolWnd* pToolWnd )
	{
		m_pToolWnd = pToolWnd;
		if ( m_pToolWnd != nullptr )
		{
			m_pTestDC.reset( new CTestDC( m_pToolWnd ) );
			m_pToolWnd->GetClientRect( &m_workAreaRect );
			m_workAreaRect.DeflateRect( m_edgeSize );

			ResetOrigin();
		}
	}

	void CTestDevice::ResetOrigin( void )
	{
		if ( IsEnabled() )
		{
			m_pToolWnd->ResetDrawPos();
			m_stripRect.TopLeft() = m_stripRect.BottomRight() = m_pToolWnd->m_drawPos;
			m_scatterCaption = false;
		}
	}

	CRect CTestDevice::GetStripTotalRect( void ) const
	{
		CRect stripRect;

		stripRect.UnionRect( &m_stripRect, m_tileRect );
		return stripRect;
	}

	void CTestDevice::SetTileAlign( TileAlign tileAlign )
	{
		if ( tileAlign != m_tileAlign )
		{
			m_tileAlign = tileAlign;

			if ( !m_stripRect.IsRectEmpty() )
				GotoNextStrip();
		}
	}

	void CTestDevice::SetSubTitle( const TCHAR* pSubTitle )
	{
		std::tstring title = s_titleBase;

		if ( !str::IsEmpty( pSubTitle ) )
			title += std::tstring( _T(" - ") ) + pSubTitle;

		ui::SetWindowText( m_pToolWnd->GetSafeHwnd(), title );
	}

	// tileSize is the size of previously drawn area
	//
	bool CTestDevice::GotoNextTile( void )
	{
		if ( !IsEnabled() )
			return false;

		m_scatterCaption = !m_scatterCaption;

		CRect tileTotalRect = m_tileRect;

		tileTotalRect.bottom = std::max( m_stripRect.bottom, tileTotalRect.bottom );

		switch ( m_tileAlign )
		{
			case TileRight:
				m_pToolWnd->m_drawPos.x += tileTotalRect.Width() + m_edgeSize.cx;
				break;
			case TileDown:
				m_pToolWnd->m_drawPos.y += tileTotalRect.Height() + m_edgeSize.cy;
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
		m_scatterCaption = false;

		if ( !IsEnabled() )
			return false;

		switch ( m_tileAlign )
		{
			case TileRight:
			case TileDown:
				m_pToolWnd->m_drawPos.x = m_edgeSize.cx;
				m_pToolWnd->m_drawPos.y = m_stripRect.bottom + m_edgeSize.cy;
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

	void CTestDevice::DrawTileFrame( COLORREF frameColor /*= color::LightGray*/, int outerEdge /*= 1*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect frameRect = m_tileRect;
		frameRect.InflateRect( outerEdge, outerEdge );

		gp::FrameRect( GetDC(), frameRect, frameColor, 50 );
	}

	void CTestDevice::DrawTileFrame( const CRect& tileRect, COLORREF frameColor /*= color::LightGray*/, int outerEdge /*= 1*/ )
	{
		StoreTileRect( tileRect );
		DrawTileFrame( frameColor, outerEdge );
	}

	void CTestDevice::DrawBitmap( HBITMAP hBitmap )
	{
		DrawBitmap( hBitmap, CBitmapInfo( hBitmap ).GetBitmapSize() );
	}

	void CTestDevice::DrawBitmap( HBITMAP hBitmap, const CSize& boundsSize, CDC* pSrcDC /*= nullptr*/ )
	{
		if ( !IsEnabled() )
			return;

		CDC* pDC = GetDC();
		CDC memDC;
		if ( nullptr == pSrcDC && memDC.CreateCompatibleDC( pDC ) )
			pSrcDC = &memDC;

		CSize bmpSize = CBitmapInfo( hBitmap ).GetBitmapSize();
		CRect boundsRect( m_pToolWnd->m_drawPos, boundsSize != CSize( 0, 0 ) ? boundsSize : bmpSize );

		CSize scaledBmpSize = ui::ShrinkToFit( boundsSize, bmpSize );

		CRect imageRect( boundsRect.TopLeft(), scaledBmpSize );
		ui::CenterRect( imageRect, boundsRect );

		int oldStretchMode = pDC->SetStretchBltMode( COLORONCOLOR );

		if ( pSrcDC != nullptr )
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

		d2d::CViewCoords viewCoords( boundsRect, imageRect );
		d2d::CBitmapCoords bmpCoords( dupTraits );

		pBitmapRT->Render( viewCoords, bmpCoords );
		StoreTileRect( boundsRect );
	}

	void CTestDevice::DrawIcon( HICON hIcon, const CSize& boundsSize, UINT flags /*= DI_NORMAL*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect iconRect = CRect( m_pToolWnd->m_drawPos, boundsSize );

		if ( !::DrawIconEx( GetDC()->GetSafeHdc(), iconRect.left, iconRect.top, hIcon, iconRect.Width(), iconRect.Height(), 0, nullptr, flags ) )
			ASSERT( false );

		DrawTileFrame( iconRect );
	}

	void CTestDevice::DrawIcon( const CIcon* pIcon, bool enabled /*= true*/ )
	{
		ASSERT_PTR( pIcon );
		if ( !IsEnabled() )
			return;

		CRect iconRect = CRect( m_pToolWnd->m_drawPos, pIcon->GetSize() );

		pIcon->Draw( *GetDC(), iconRect.TopLeft(), enabled );
		DrawTileFrame( iconRect );
		DrawTileCaption( str::Format( _T("%dx%d, %d-bit%s"), iconRect.Width(), iconRect.Height(), pIcon->GetBitsPerPixel(),
									  pIcon->HasAlpha() ? _T("+A") : str::GetEmpty().c_str() ) );
	}

	void CTestDevice::DrawImage( CImageList* pImageList, int index, UINT style /*= ILD_TRANSPARENT*/ )
	{
		if ( !IsEnabled() )
			return;

		CRect imageRect( m_pToolWnd->m_drawPos, gdi::GetImageIconSize( *pImageList ) );

		pImageList->Draw( GetDC(), index, imageRect.TopLeft(), style );
		DrawTileFrame( imageRect );
	}

	void CTestDevice::DrawImageList( CImageList* pImageList, bool putTags /*= false*/, UINT style /*= ILD_TRANSPARENT*/ )
	{
		if ( !IsEnabled() )
			return;

		ASSERT_PTR( pImageList );

		UINT imageCount = pImageList->GetImageCount();
		CSize imageSize = gdi::GetImageIconSize( *pImageList );

		CRect imageRect = CRect( m_pToolWnd->m_drawPos, imageSize );
		CRect boundsRect = imageRect;			// bounds drawn

		CDC* pDC = GetDC();
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );

		for ( UINT i = 0; i != imageCount; ++i, imageRect.OffsetRect( 0, imageSize.cy ) )
		{
			pImageList->Draw( pDC, i, imageRect.TopLeft(), style );
			boundsRect |= imageRect;

			if ( putTags )
			{
				std::tstring tag = str::Format( _T("[%d]"), i );
				int width = ui::GetTextSize( pDC, tag.c_str() ).cx + 5;

				CRect textRect = imageRect;
				textRect.left = imageRect.right + 5;
				textRect.right = textRect.left + width;

				pDC->DrawText( tag.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

				boundsRect |= textRect;
			}
		}

		pDC->SetBkMode( oldBkMode );

		DrawTileFrame( boundsRect );
	}

	bool CTestDevice::DrawWideBitmap( HBITMAP hBitmap, const CSize& glyphSize )
	{	// draw a wide bitmap (e.g. CMFCToolBarImages::GetImageWell), spanning over multiple rows until the entire bitmap width is diplayed
		if ( !IsEnabled() || nullptr == hBitmap )
			return false;

		enum { RowSpacingY = 5 };

		CSize fullSize = gdi::GetBitmapSize( hBitmap );
		size_t columnCount = m_workAreaRect.Width() / glyphSize.cx;
		int rowMaxWidth = static_cast<int>( columnCount * glyphSize.cx );

		CDC* pDC = GetDC();
		CDC memDC;

		if ( !memDC.CreateCompatibleDC( pDC ) )
			return false;

		CScopedGdiObj scopedBitmap( &memDC, hBitmap );

		CRect boundsRect( m_pToolWnd->m_drawPos, CSize( 0, 0 ) );
		CRect srcRect( 0, 0, 0, fullSize.cy );

		for ( int srcWidth = fullSize.cx; srcWidth != 0; )
		{
			ASSERT( srcWidth > 0 );

			int rowWidth = utl::min( srcWidth, rowMaxWidth );
			CRect rowRect( m_pToolWnd->m_drawPos, CSize( rowWidth, fullSize.cy ) );

			srcRect.left = srcRect.right;
			srcRect.right = srcRect.left + rowWidth;

			boundsRect |= rowRect;

			ut::DrawBitmap( pDC, rowRect, &memDC, srcRect );
			DrawTileFrame( rowRect, color::LightGray );

			srcWidth -= srcRect.Width();
			m_pToolWnd->m_drawPos.y += rowRect.Height() + RowSpacingY;
		}

		m_workAreaRect.top = boundsRect.bottom;
		m_tileRect.right = m_workAreaRect.right;
		return true;
	}

	void CTestDevice::DrawImages( CMFCToolBarImages* pImages )
	{
		if ( !IsEnabled() || nullptr == pImages || nullptr == pImages->GetImageWell() )
			return;

		HBITMAP hBitmap = pImages->GetImageWell();
		CSize glyphSize = pImages->GetImageSize();

		ASSERT_PTR( hBitmap );
		DrawWideBitmap( hBitmap, glyphSize );		// draw wide bitmap spanning on multiple rows
		DrawTileCaption( str::Format( _T("%d total images"), pImages->GetCount() ) );

		m_workAreaRect.top = m_stripRect.bottom + 10;
		m_pToolWnd->m_drawPos = m_workAreaRect.TopLeft();
	}

	void CTestDevice::DrawImagesDetails( CMFCToolBarImages* pImages )
	{
		if ( !IsEnabled() )
			return;

		ASSERT_PTR( pImages );

		enum { FrameSpacingX = 1, FrameSpacingY = 1, TextSpacingX = 5 };
		const CSize frameSize( FrameSpacingX * 2, FrameSpacingY * 2 );

		UINT imageCount = pImages->GetCount();
		CSize imageSize = pImages->GetImageSize();
		CSize itemSize = ui::InflateSize( imageSize, frameSize );

		const int topDrawArea = m_pToolWnd->m_drawPos.y;
		CRect itemRect = CRect( m_pToolWnd->m_drawPos, itemSize );
		CRect boundsRect = itemRect;			// bounds drawn

		CAfxDrawState drawState;
		if ( !pImages->PrepareDrawImage( drawState ) )
			return;

		const mfc::CImageCommandLookup* pImageCmd = mfc::CImageCommandLookup::Instance();
		CDC* pDC = GetDC();

		for ( UINT i = 0; i != imageCount; ++i )
		{
			CRect imageRect = itemRect;

			imageRect.DeflateRect( frameSize );
			ut::DrawGlyphFramePoints( pDC, imageRect );

			pImages->Draw( pDC, imageRect.left, imageRect.top, i );

			boundsRect |= itemRect;

			{
				std::tstring tag = str::Format( _T("[%d]"), i /*+ 1*/ );
				enum { MaxDisplayLength = 17 };

				if ( const std::tstring* pCmdName = pImageCmd->FindCommandNameByPos( i ) )
					stream::Tag( tag, str::GetClampHead( *pCmdName, MaxDisplayLength, _T("~") ), _T("  ") );

				int width = ui::GetTextSize( GetDC(), tag.c_str() ).cx + TextSpacingX;

				CRect textRect = itemRect;
				textRect.left = itemRect.right + 5;
				textRect.right = textRect.left + width;
				GetDC()->DrawText( tag.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

				boundsRect |= textRect;
			}

			itemRect.OffsetRect( 0, itemSize.cy );

			if ( itemRect.bottom >= m_workAreaRect.bottom )		// bottom overflow?
			{	// start a new column
				m_pToolWnd->m_drawPos = CPoint( boundsRect.right, topDrawArea /*m_workAreaRect.top*/ );
				itemRect = CRect( m_pToolWnd->m_drawPos, itemSize );
			}
		}

		pImages->EndDrawImage( drawState );

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

	void CTestDevice::DrawHeadline( const TCHAR* pHeadline, COLORREF textColor /*= color::Blue*/ )
	{
		enum { TitleSpacingY = 4, DtFormat = DT_NOPREFIX | DT_SINGLELINE };

		if ( m_stripRect.Height() != 0 )
			m_stripRect.top = m_stripRect.bottom = m_stripRect.bottom + TitleSpacingY;		// collapse to the bottom

		m_stripRect.right = m_workAreaRect.right;

		std::tstring text = _T("● "); text += pHeadline;

		CDC* pDC = GetDC();
		CScopedGdi<CFont> scFont( pDC, &m_pToolWnd->m_headlineFont );
		COLORREF oldTextColor = pDC->SetTextColor( textColor );
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );
		int textHeight = ui::GetTextSize( pDC, text.c_str() ).cy;

		textHeight = utl::max( pDC->GetTextExtent( text.c_str(), (int)text.length() ).cy, textHeight );		// compensate since ui::GetTextSize() not acurate with vertical size
		m_stripRect.bottom += textHeight;

		GetDC()->DrawText( text.c_str(), (int)text.length(), &m_stripRect, DT_END_ELLIPSIS | DtFormat );
		//gp::FrameRect( pDC, m_stripRect, color::LightGray, 50 );
		pDC->SetTextColor( oldTextColor );
		pDC->SetBkMode( oldBkMode );

		m_pToolWnd->m_drawPos.y += m_stripRect.Height() + TitleSpacingY;
		m_stripRect.TopLeft() = m_stripRect.BottomRight() = m_pToolWnd->m_drawPos;
	}

	void CTestDevice::DrawTileCaption( const std::tstring& text, bool ellipsys /*= false*/ )
	{
		if ( !IsEnabled() || text.empty() )
			return;

		ASSERT( !m_tileRect.IsRectEmpty() );

		enum { TextEdge = 5, ScatterSpacing = 2 };

		CDC* pDC = GetDC();

		static const std::tstring s_spacing = _T(" ");
		std::tstring displayText = text;
		DWORD dtFormat = DT_NOPREFIX | DT_SINGLELINE;

		if ( OPAQUE == pDC->GetBkMode() )
			displayText = s_spacing + text + s_spacing;		// draw background around the text label

		CSize textSize = ui::GetTextSize( pDC, displayText.c_str(), dtFormat );

		//textSize.cx = std::min( (long)m_tileRect.Width(), textSize.cx );
		textSize.cy = std::max( pDC->GetTextExtent( displayText.c_str(), (int)displayText.length() ).cy, textSize.cy );	// compensate since ui::GetTextSize() not acurate with vertical size

		CRect textRect( CPoint( 0, 0 ), textSize );

		if ( ut::TileRight == m_tileAlign )
		{
			ui::AlignRectOutside( textRect, m_tileRect, H_AlignCenter | V_AlignBottom, CSize( 0, TextEdge ) );

			if ( m_scatterCaption )
				textRect.OffsetRect( 0, textRect.Height() + ScatterSpacing );

			m_stripRect.bottom = std::max( textRect.bottom, m_stripRect.bottom );
			SetFlag( dtFormat, DT_END_ELLIPSIS, ellipsys );
		}
		else
		{
			ui::AlignRectOutside( textRect, m_tileRect, H_AlignRight | V_AlignCenter, CSize( TextEdge, 0 ) );
			m_stripRect.right = std::max( textRect.right, m_stripRect.right );
		}

		pDC->DrawText( displayText.c_str(), (int)displayText.length(), &textRect, dtFormat );
	}


	COLORREF CTestDevice::GetBitmapFrameColor( const CSize& bmpSize, const CSize& scaledBmpSize )
	{
		if ( scaledBmpSize.cx < bmpSize.cx )			// shrunk
			return color::Red;
		else if ( scaledBmpSize.cx > bmpSize.cx )		// stretched
			return color::LightBlue;

		return color::LightGray;
	}

} //namespace ut


#endif //USE_UT
