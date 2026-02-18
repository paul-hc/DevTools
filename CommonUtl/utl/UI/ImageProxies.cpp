
#include "pch.h"
#include "ImageProxies.h"
#include "DibDraw.h"
#include "Icon.h"
#include "Color.h"
#include "StdColors.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBitmapProxy implementation

void ui::IImageProxy::FillBackground( CDC* pDC, const CPoint& pos, COLORREF bkColor ) const
{
	CBrush brush( bkColor );
	HGDIOBJ hOldBrush = ::SelectObject( *pDC, brush );

	pDC->PatBlt( pos.x, pos.y, GetSize().cx, GetSize().cy, PATCOPY );
	::SelectObject( *pDC, hOldBrush );
}


// CBitmapProxy implementation

CBitmapProxy::CBitmapProxy( HBITMAP hBitmap /*= nullptr*/ )
	: m_hBitmapList( hBitmap )
	, m_index( NoImage )
	, m_size( 0, 0 )
{
	if ( m_hBitmapList != nullptr )
	{
		m_index = 0;
		m_size = gdi::GetBitmapSize( m_hBitmapList );
		Init();
	}
}

CBitmapProxy::CBitmapProxy( HBITMAP hBitmapList, int bitmapIndex, const CSize& size )
	: m_hBitmapList( hBitmapList )
	, m_index( bitmapIndex )
	, m_size( size )
{
	ASSERT( hBitmapList != nullptr && bitmapIndex >= 0 && size.cx != 0 && size.cy != 0 );
	Init();
}

void CBitmapProxy::Init( void )
{
	if ( m_hBitmapList != nullptr && gdi::IsDibSection( m_hBitmapList ) )
	{
		m_pDibTraits.reset( new CDibSectionTraits( m_hBitmapList ) );
		ENSURE( m_pDibTraits->IsValid() );
	}
}

bool CBitmapProxy::HasTransparency( void ) const override
{
	return m_pDibTraits.get() != nullptr &&m_pDibTraits->HasAlphaChannel();
}

void CBitmapProxy::Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor /*= CLR_NONE*/ ) const override
{
	ASSERT( !IsEmpty() );

	CDC memDC;

	if ( memDC.CreateCompatibleDC( pDC ) )
	{
		CScopedGdiObj scopedBitmap( &memDC, m_hBitmapList );
		CScopedPalette scopedPalette( pDC, m_pDibTraits.get() != nullptr ? m_pDibTraits->MakeColorPalette( pDC ) : nullptr );

		CPoint srcPos( m_index * m_size.cx, 0 );

		if ( ui::IsUndefinedColor( transpColor ) )
		{
			pDC->BitBlt( pos.x, pos.y, m_size.cx, m_size.cy, &memDC, srcPos.x, srcPos.y, SRCCOPY );
		}
		else
		{
			pDC->TransparentBlt( pos.x, pos.y, m_size.cx, m_size.cy, &memDC,
								 srcPos.x, srcPos.y, m_size.cx, m_size.cy,
								 ui::GetFallbackSysColor( transpColor, COLOR_3DFACE ) );
		}
	}
}

void CBitmapProxy::DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor /*= CLR_NONE*/ ) const override
{
	transpColor;
	ASSERT( !IsEmpty() );

	CDC memDC;
	if ( memDC.CreateCompatibleDC( pDC ) )
	{
		CScopedGdiObj scopedBitmap( &memDC, m_hBitmapList );
		CDC monoDC;
		CBitmap maskBitmap;

		if ( monoDC.CreateCompatibleDC( &memDC ) &&
			 maskBitmap.CreateBitmap( m_size.cx, m_size.cy, 1, 1, nullptr ) )
		{
			CScopedGdi<CBitmap> scopedMonoBitmap( &monoDC, &maskBitmap );

			CreateMonochromeMask( &memDC, &monoDC );

			COLORREF oldBkColor = pDC->SetBkColor( color::White );
			COLORREF oldTextColor = pDC->SetTextColor( color::Black );
			CPoint srcPos( m_index * m_size.cx, 0 );

			// draw highlight shadow where we have zeros in our mask
			HGDIOBJ hOldBrush = ::SelectObject( *pDC, GetSysColorBrush( COLOR_3DHILIGHT ) );
			pDC->BitBlt( pos.x + 1, pos.y + 1, m_size.cx, m_size.cy, &monoDC, srcPos.x, srcPos.y, ROP_PSDPxax );

			// draw shadow where we have zeros in our mask
			::SelectObject( *pDC, GetSysColorBrush( COLOR_3DSHADOW ) );
			pDC->BitBlt( pos.x, pos.y, m_size.cx, m_size.cy, &monoDC, srcPos.x, srcPos.y, ROP_PSDPxax );

			pDC->SetBkColor( oldBkColor );
			pDC->SetTextColor( oldTextColor );
			::SelectObject( *pDC, hOldBrush );
		}
	}
}

void CBitmapProxy::CreateMonochromeMask( CDC* pMemDC, CDC* pMonoDC ) const
{
	pMonoDC->PatBlt( 0, 0, m_size.cx, m_size.cy, WHITENESS );

	CPoint srcPos( m_index * m_size.cx, 0 );

	// create mask based on the button bitmap
	pMemDC->SetBkColor( GetSysColor( COLOR_BTNFACE ) );
	pMonoDC->BitBlt( 0, 0, m_size.cx, m_size.cy, pMemDC, srcPos.x, srcPos.y, SRCCOPY );

	pMemDC->SetBkColor( GetSysColor( COLOR_BTNHILIGHT ) );
	pMonoDC->BitBlt( 0, 0, m_size.cx, m_size.cy, pMemDC, srcPos.x, srcPos.y, SRCPAINT );
}


// CIconProxy implementation

CIconProxy::CIconProxy( const CIcon* pIcon /*= nullptr*/ )
	: m_pIcon( const_cast<CIcon*>( pIcon ) )
	, m_hasOwnership( false )
{
}

CIconProxy::CIconProxy( HICON hIcon )
	: m_pIcon( CIcon::LoadNewIcon( hIcon ) )
	, m_hasOwnership( true )
{
}

CIconProxy::~CIconProxy()
{
	if ( m_hasOwnership )
		delete m_pIcon;
}

const CSize& CIconProxy::GetSize( void ) const override
{
	return m_pIcon->GetSize();
}

void CIconProxy::Draw( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	if ( m_pIcon != nullptr )
		m_pIcon->Draw( *pDC, pos );
}

void CIconProxy::DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	if ( m_pIcon != nullptr )
		m_pIcon->Draw( *pDC, pos, false );
}


// CImageListProxy implementation

CImageListProxy::CImageListProxy( CImageList* pImageList /*= nullptr*/, int imageIndex /*= NoImage*/, int overlayMask /*= NoOverlayMask*/ )
	: m_pImageList( nullptr )
	, m_imageIndex( imageIndex )
	, m_overlayMask( NoOverlayMask )
	, m_pExternalOverlay( nullptr )
	, m_imageSize( 0, 0 )
{
	SetOverlayMask( overlayMask );		// do some validation
	SetImageList( pImageList );
}

void CImageListProxy::Reset( CImageList* pImageList, int imageIndex )
{
	SetImageList( pImageList );
	m_imageIndex = imageIndex;
}

void CImageListProxy::SetImageList( CImageList* pImageList )
{
	m_pImageList = pImageList;

	if ( m_pImageList != nullptr )
		m_imageSize = gdi::GetImageIconSize( m_pImageList->GetSafeHandle() );
}

void CImageListProxy::Draw( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	ASSERT( !IsEmpty() );

	VERIFY( const_cast<CImageList*>( m_pImageList )->Draw( pDC, m_imageIndex, pos, ILD_TRANSPARENT | INDEXTOOVERLAYMASK( m_overlayMask ) ) );

	if ( HasExternalOverlay() )
		m_pExternalOverlay->Draw( pDC, pos, ILD_TRANSPARENT );
}

void CImageListProxy::DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	ASSERT( !IsEmpty() );

	DrawDisabledImpl( pDC, pos, ILD_TRANSPARENT | INDEXTOOVERLAYMASK( m_overlayMask ) );

	if ( HasExternalOverlay() )
		m_pExternalOverlay->DrawDisabled( pDC, pos, ILD_TRANSPARENT );
}

void CImageListProxy::DrawDisabledImpl( CDC* pDC, const CPoint& pos, UINT style ) const
{
	ASSERT( !IsEmpty() );
	gdi::DrawDisabledImage( pDC, *m_pImageList, m_imageSize, m_imageIndex, pos, m_imageSize, style );
}

void CImageListProxy::DrawDisabledImpl_old( CDC* pDC, const CPoint& pos, UINT style ) const
{
	ASSERT( !IsEmpty() );

	CDC memDC;
	if ( !memDC.CreateCompatibleDC( pDC ) )
	{
		CIcon icon( m_pImageList->ExtractIcon( m_imageIndex ), m_imageSize );
		icon.DrawDisabled( *pDC, pos );
		return;
	}

	BITMAPINFO bmpInfo;
	ZeroMemory( &bmpInfo, sizeof( BITMAPINFO ) );

	bmpInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmpInfo.bmiHeader.biWidth = m_imageSize.cx;
	bmpInfo.bmiHeader.biHeight = m_imageSize.cy;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biHeight * 4;

	bool hasAlphaChannel = false;		// we can use AlphaBlend if the alpha channel is defined for 32bpp icons
	BYTE* pPixels;

	if ( HBITMAP hDib = ::CreateDIBSection( memDC, &bmpInfo, DIB_RGB_COLORS, (void**)&pPixels, nullptr, 0 ) )
	{
		CScopedGdiObj scopedBitmap( &memDC, hDib );

		Draw( &memDC, CPoint( 0, 0 ), style );

		// convert to grayscale
		enum ComponentIndex { Red, Green, Blue, Alpha };
		typedef BYTE* iterator;

		for ( iterator pPixel = pPixels, pEnd = pPixel + bmpInfo.bmiHeader.biSizeImage; pPixel < pEnd; pPixel += 4 )
		{
			pPixel[ Red ] = pPixel[ Green ] = pPixel[ Blue ] = ui::GetLuminance( pPixel[ Red ], pPixel[ Green ], pPixel[ Blue ] );

			if ( pPixel[ Alpha ] != 0 )
				hasAlphaChannel = true;
		}

		if ( hasAlphaChannel )
		{
			static const BYTE transparent25 = 0x3F, transparent50 = 0x7F, transparent75 = 0xBF;

			BLENDFUNCTION blendFunction;
			blendFunction.BlendOp = AC_SRC_OVER;
			blendFunction.BlendFlags = 0;
			blendFunction.SourceConstantAlpha = transparent50;
			blendFunction.AlphaFormat = AC_SRC_ALPHA;  // use bitmap alpha

			if ( !pDC->AlphaBlend( pos.x, pos.y, m_imageSize.cx, m_imageSize.cy, &memDC, 0, 0, m_imageSize.cx, m_imageSize.cy, blendFunction ) )
				hasAlphaChannel = false;
		}
	}

	if ( !hasAlphaChannel )
	{	// no Alpha channel is present, use the standard embossing
		// black: pattern
		// white: destintation (transparent)

		CBitmap monochromeBitmap;
		monochromeBitmap.CreateBitmap( m_imageSize.cx, m_imageSize.cy, 1, 1, nullptr );		// for color: monochromeBitmap.CreateCompatibleBitmap( pDC, cx, cy );

		CScopedGdi<CBitmap> scopedBitmap( &memDC, &monochromeBitmap );
		CPoint posMem( 0, 0 );

		memDC.PatBlt( posMem.x, posMem.y, m_imageSize.cx, m_imageSize.cy, WHITENESS );		// fill background white first
		Draw( &memDC, posMem, style );

		COLORREF oldBkColor = pDC->SetBkColor( RGB( 255, 255, 255 ) );
		CBrush brushHighlight( GetSysColor( COLOR_3DHIGHLIGHT ) );
		CBrush* pOldBrush = pDC->SelectObject( &brushHighlight );

		// draw with highlight color at offset (1, 1) - this will not be visible on white background
		pDC->BitBlt( pos.x + 1, pos.y + 1, m_imageSize.cx, m_imageSize.cy, &memDC, 0, 0, ROP_PSDPxax );

		CBrush brushShadow( GetSysColor( COLOR_3DSHADOW ) );
		pDC->SelectObject( &brushShadow );
		pDC->BitBlt( pos.x, pos.y, m_imageSize.cx, m_imageSize.cy, &memDC, 0, 0, ROP_PSDPxax ); // draw with shadow color
		pDC->SelectObject( pOldBrush );
		pDC->SetBkColor( oldBkColor );
	}
}


// CImageListStripProxy implementation

CImageListStripProxy::CImageListStripProxy( const CImageList* pImageList )
	: m_pImageList( const_cast<CImageList*>( pImageList ) )
	, m_imageCount( m_pImageList->GetImageCount() )
	, m_imageSize( gdi::GetImageIconSize( m_pImageList->GetSafeHandle() ) )
	, m_stripSize( m_imageSize.cx * m_imageCount, m_imageSize.cy )
{
	ASSERT_PTR( m_pImageList );
}

void CImageListStripProxy::Draw( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	ASSERT( !IsEmpty() );

	CImageList* pImageList = const_cast<CImageList*>( m_pImageList );
	CPoint imagePos = pos;

	for ( int i = 0; i != m_imageCount; ++i, imagePos.x += m_imageSize.cx )
		VERIFY( pImageList->Draw( pDC, i, imagePos, ILD_TRANSPARENT ) );
}

void CImageListStripProxy::DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	ASSERT( !IsEmpty() );

	CImageList* pImageList = const_cast<CImageList*>( m_pImageList );
	CPoint imagePos = pos;

	for ( int i = 0; i != m_imageCount; ++i, imagePos.x += m_imageSize.cx )
		gdi::DrawDisabledImage( pDC, *pImageList, m_imageSize, i, imagePos, m_imageSize, ILD_TRANSPARENT );
}


// CColorBoxImage implementation

CColorBoxImage::CColorBoxImage( COLORREF color, const CSize& size /*= CSize( AutoTextSize, AutoTextSize )*/ )
	: m_color( color )
	, m_size( size )
{
}

CColorBoxImage::~CColorBoxImage()
{
}

void CColorBoxImage::SizeToText( CDC* pDC ) override
{
	if ( AutoTextSize == m_size.cx || AutoTextSize == m_size.cy )
	{
		const int textHeight = ui::GetTextSize( pDC, _T("Xg") ).cy;

		if ( AutoTextSize == m_size.cx )
			m_size.cx = textHeight;

		if ( AutoTextSize == m_size.cy )
			m_size.cy = textHeight;
	}
}

void CColorBoxImage::Draw( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	DrawImpl( pDC, pos, true );
}

void CColorBoxImage::DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF /*transpColor = CLR_NONE*/ ) const override
{
	DrawImpl( pDC, pos, false );
}

void CColorBoxImage::DrawImpl( CDC* pDC, const CPoint& pos, bool enabled ) const
{
	CRect rect( pos, m_size );
	CPen pen( PS_SOLID, 1, GetSysColor( enabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT ) );
	CBrush brush;

	if ( m_color != CLR_NONE )
		brush.CreateSolidBrush( m_color );
	else
		brush.CreateHatchBrush( HS_BDIAGONAL, GetSysColor( COLOR_GRAYTEXT ) );

	CBrush* pOldBrush = pDC->SelectObject( &brush );
	CPen* pOldPen = pDC->SelectObject( &pen );

	pDC->RoundRect( &rect, CPoint( 3, 3 ) );

	pDC->SelectObject( pOldBrush );
	pDC->SelectObject( pOldPen );
}
