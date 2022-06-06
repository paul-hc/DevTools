
#include "stdafx.h"
#include "ImageProxy.h"
#include "DibDraw.h"
#include "Icon.h"
#include "Color.h"
#include "StdColors.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CImageProxy implementation

CImageProxy::CImageProxy( CImageList* pImageList /*= NULL*/, int index /*= NoImage*/, int overlayMask /*= NoOverlayMask*/ )
	: m_pImageList( pImageList )
	, m_index( index )
	, m_overlayMask( NoOverlayMask )
	, m_pExternalOverlay( NULL )
	, m_size( 0, 0 )
{
	SetOverlayMask( overlayMask ); // do some validation
}

bool CImageProxy::IsEmpty( void ) const
{
	return NULL == m_pImageList || m_index < 0;
}

const CSize& CImageProxy::GetSize( void ) const
{
	if ( 0 == m_size.cx && 0 == m_size.cy )
		if ( !IsEmpty() )
		{
			int cx, cy;
			VERIFY( ::ImageList_GetIconSize( m_pImageList->GetSafeHandle(), &cx, &cy ) );
			m_size.cx = cx;
			m_size.cy = cy;
		}

	return m_size;
}

void CImageProxy::Set( CImageList* pImageList, int index )
{
	m_pImageList = pImageList;
	m_index = index;
	m_size.cx = m_size.cy = 0;
}

void CImageProxy::Draw( CDC* pDC, const CPoint& pos, UINT style /*= ILD_TRANSPARENT*/ ) const
{
	ASSERT( !IsEmpty() );

	VERIFY( const_cast<CImageList*>( m_pImageList )->Draw( pDC, m_index, pos, style | INDEXTOOVERLAYMASK( m_overlayMask ) ) );

	if ( HasExternalOverlay() )
		m_pExternalOverlay->Draw( pDC, pos, style );
}

void CImageProxy::DrawDisabled( CDC* pDC, const CPoint& pos, UINT style /*= ILD_TRANSPARENT*/ ) const
{
	DrawDisabledImpl( pDC, pos, style | INDEXTOOVERLAYMASK( m_overlayMask ) );

	if ( HasExternalOverlay() )
		m_pExternalOverlay->DrawDisabled( pDC, pos, style );
}

void CImageProxy::DrawDisabledImpl( CDC* pDC, const CPoint& pos, UINT style ) const
{
	ASSERT( !IsEmpty() );
	GetSize();
	gdi::DrawDisabledImage( pDC, *m_pImageList, m_size, m_index, pos, m_size, style );
}

void CImageProxy::DrawDisabledImpl_old( CDC* pDC, const CPoint& pos, UINT style ) const
{
	ASSERT( !IsEmpty() );
	GetSize();

	CDC memDC;
	if ( !memDC.CreateCompatibleDC( pDC ) )
	{
		CIcon icon( m_pImageList->ExtractIcon( m_index ), m_size );
		icon.DrawDisabled( *pDC, pos );
		return;
	}

	BITMAPINFO bmpInfo;
	ZeroMemory( &bmpInfo, sizeof( BITMAPINFO ) );
	bmpInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmpInfo.bmiHeader.biWidth = m_size.cx;
	bmpInfo.bmiHeader.biHeight = m_size.cy;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biHeight * 4;

	bool hasAlphaChannel = false;		// we can use AlphaBlend if the alpha channel is defined for 32bpp icons
	BYTE* pPixels;
	if ( HBITMAP hDib = ::CreateDIBSection( memDC, &bmpInfo, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0 ) )
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

			if ( !pDC->AlphaBlend( pos.x, pos.y, m_size.cx, m_size.cy, &memDC, 0, 0, m_size.cx, m_size.cy, blendFunction ) )
				hasAlphaChannel = false;
		}
	}

	if ( !hasAlphaChannel )
	{	// no Alpha channel is present, use the standard embossing
		// black: pattern
		// white: destintation (transparent)

		CBitmap monochromeBitmap;
		monochromeBitmap.CreateBitmap( m_size.cx, m_size.cy, 1, 1, NULL );		// for color: monochromeBitmap.CreateCompatibleBitmap( pDC, cx, cy );

		CScopedGdi< CBitmap > scopedBitmap( &memDC, &monochromeBitmap );
		CPoint posMem( 0, 0 );

		memDC.PatBlt( posMem.x, posMem.y, m_size.cx, m_size.cy, WHITENESS );	// fill background white first
		Draw( &memDC, posMem, style );

		COLORREF oldBkColor = pDC->SetBkColor( RGB( 255, 255, 255 ) );
		CBrush brushHighlight( GetSysColor( COLOR_3DHIGHLIGHT ) );
		CBrush* pOldBrush = pDC->SelectObject( &brushHighlight );

		// draw with highlight color at offset (1, 1) - this will not be visible on white background
		pDC->BitBlt( pos.x + 1, pos.y + 1, m_size.cx, m_size.cy, &memDC, 0, 0, ROP_PSDPxax );

		CBrush brushShadow( GetSysColor( COLOR_3DSHADOW ) );
		pDC->SelectObject( &brushShadow );
		pDC->BitBlt( pos.x, pos.y, m_size.cx, m_size.cy, &memDC, 0, 0, ROP_PSDPxax ); // draw with shadow color
		pDC->SelectObject( pOldBrush );
		pDC->SetBkColor( oldBkColor );
	}
}


// CBitmapProxy implementation

CBitmapProxy::CBitmapProxy( void )
	: m_index( NoImage )
	, m_size( 0, 0 )
{
}

CBitmapProxy::CBitmapProxy( HBITMAP hBitmapList, int bitmapIndex, const CSize& size )
	: m_hBitmapList( hBitmapList )
	, m_index( bitmapIndex )
	, m_size( size )
{
	ASSERT( hBitmapList != NULL && bitmapIndex >= 0 && size.cx != 0 && size.cy != 0 );
}

bool CBitmapProxy::IsEmpty( void ) const
{
	return NULL == m_hBitmapList != NULL || m_index < 0;
}

const CSize& CBitmapProxy::GetSize( void ) const
{
	return m_size;
}

void CBitmapProxy::Draw( CDC* pDC, const CPoint& pos, UINT style /*= ILD_TRANSPARENT*/ ) const
{
	style;
	ASSERT( !IsEmpty() );

	CDC memDC;
	if ( memDC.CreateCompatibleDC( pDC ) )
	{
		CScopedGdiObj scopedBitmap( &memDC, m_hBitmapList );

		pDC->TransparentBlt( pos.x, pos.y, m_size.cx, m_size.cy, &memDC,
							 m_index * m_size.cx, 0, m_size.cx, m_size.cy, GetSysColor( COLOR_3DFACE ) );
	}
}

void CBitmapProxy::DrawDisabled( CDC* pDC, const CPoint& pos, UINT /*style = ILD_TRANSPARENT*/ ) const
{
	ASSERT( !IsEmpty() );

	CDC memDC;
	if ( memDC.CreateCompatibleDC( pDC ) )
	{
		CScopedGdiObj scopedBitmap( &memDC, m_hBitmapList );
		CDC monoDC;
		CBitmap maskBitmap;

		if ( monoDC.CreateCompatibleDC( &memDC ) &&
			 maskBitmap.CreateBitmap( m_size.cx, m_size.cy, 1, 1, NULL ) )
		{
			CScopedGdi< CBitmap > scopedMonoBitmap( &monoDC, &maskBitmap );

			CreateMonochromeMask( &memDC, &monoDC );

			COLORREF oldBkColor = pDC->SetBkColor( color::White );
			COLORREF oldTextColor = pDC->SetTextColor( color::Black );

			// draw highlight shadow where we have zeros in our mask
			HGDIOBJ hOldBrush = SelectObject( *pDC, GetSysColorBrush( COLOR_3DHILIGHT ) );
			pDC->BitBlt( pos.x + 1, pos.y + 1, m_size.cx, m_size.cy, &monoDC, 0, 0, ROP_PSDPxax );

			// draw shadow where we have zeros in our mask
			SelectObject( *pDC, GetSysColorBrush( COLOR_3DSHADOW ) );
			pDC->BitBlt( pos.x, pos.y, m_size.cx, m_size.cy, &monoDC, 0, 0, ROP_PSDPxax );

			pDC->SetBkColor( oldBkColor );
			pDC->SetTextColor( oldTextColor );
			SelectObject( *pDC, hOldBrush );
		}
	}
}

void CBitmapProxy::CreateMonochromeMask( CDC* pMemDC, CDC* pMonoDC ) const
{
	pMonoDC->PatBlt( 0, 0, m_size.cx, m_size.cy, WHITENESS );

	int xSource = m_index * m_size.cx;

	// create mask based on the button bitmap
	pMemDC->SetBkColor( GetSysColor( COLOR_BTNFACE ) );
	pMonoDC->BitBlt( 0, 0, m_size.cx, m_size.cy, pMemDC, xSource, 0, SRCCOPY );

	pMemDC->SetBkColor( GetSysColor( COLOR_BTNHILIGHT ) );
	pMonoDC->BitBlt( 0, 0, m_size.cx, m_size.cy, pMemDC, xSource, 0, SRCPAINT );
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

void CColorBoxImage::SizeToText( CDC* pDC )
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

bool CColorBoxImage::IsEmpty( void ) const
{
	return false;
}

const CSize& CColorBoxImage::GetSize( void ) const
{
	return m_size;
}

void CColorBoxImage::Draw( CDC* pDC, const CPoint& pos, UINT /*style = ILD_TRANSPARENT*/ ) const
{
	DrawImpl( pDC, pos, true );
}

void CColorBoxImage::DrawDisabled( CDC* pDC, const CPoint& pos, UINT /*style = ILD_TRANSPARENT*/ ) const
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
