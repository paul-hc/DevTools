
#include "stdafx.h"
#include "Imaging.h"
#include "ImagingGdiPlus.h"
#include "ImagingWic.h"
#include "DibPixels.h"
#include "FileSystem.h"
#include <afxglobals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	CDibMeta LoadImageFromFile( const TCHAR* pFilePath, ui::ImagingApi api /*= ui::WicApi*/, UINT framePos /*= 0*/ )
	{
		switch ( api )
		{
			default: ASSERT( false );
			case ui::WicApi:
				return wic::LoadImageFromFile( pFilePath, framePos );
			case ui::GpApi:
				ASSERT( 0 == framePos );
				return gp::LoadImageFromFile( pFilePath );
		}
	}

	CDibMeta LoadPng( UINT pngId, ui::ImagingApi api /*= ui::WicApi*/, bool mapTo3DColors /*= false*/ )
	{
		switch ( api )
		{
			default: ASSERT( false );
			case ui::WicApi:
				return wic::LoadPng( MAKEINTRESOURCE( pngId ), mapTo3DColors );
			case ui::GpApi:
				return gp::LoadPng( MAKEINTRESOURCE( pngId ), mapTo3DColors );
		}
	}

} //namespace ui


namespace gdi
{
	const TCHAR g_imageFileFilter[] =
		_T("Image Files (*.bmp;*.dib;*.png;*.jpg;*.gif;*.jpeg;*.tif;*.tiff;*.wmp;*.ico;*.cur)|*.bmp;*.dib;*.png;*.gif;*.jpg;*.jpeg;*.jpe;*.jfif;*.tif;*.tiff;*.wmp;*.ico;*.cur|")
		_T("Windows Bitmaps (*.bmp;*.dib)|*.bmp;*.dib|")
		_T("PNG: Portable Network Graphics (*.png)|*.png|")
		_T("GIF: Graphics Interchange Format (*.gif)|*.gif|")
		_T("JPEG Files (*.jpg;*.jpeg;*.jpe;*.jfif)|*.jpg;*.jpeg;*.jpe;*.jfif|")
		_T("TIFF Files (*.tif;*.tiff)|*.tif;*.tiff|")
		_T("WMP Files (*.wmp)|*.wmp|")
		_T("Icon & Cursor (*.ico;*.cur)|*.ico;*.cur|")
		_T("High Definition Photo (*.wdp;*.mdp;*.hdp)|*.wdp;*.mdp;*.hdp|")
		_T("All Files (*.*)|*.*||");


	CDibMeta LoadBitmapAsDib( const TCHAR* pBmpName, bool mapTo3DColors /*= false*/ )
	{
		ASSERT_PTR( pBmpName );

		HINSTANCE hResInst = NULL;
		UINT flags = 0;

		if ( IS_INTRESOURCE( pBmpName ) )
			hResInst = CScopedResInst::Find( pBmpName, RT_BITMAP );
		else if ( fs::IsValidFile( pBmpName ) )
			SetFlag( flags, LR_LOADFROMFILE );

		if ( mapTo3DColors )
			SetFlag( flags, LR_LOADMAP3DCOLORS );

		CDibMeta dibMeta( (HBITMAP)::LoadImage( hResInst, pBmpName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | flags ) );
		if ( dibMeta.IsValid() )
		{
			if ( mapTo3DColors )
				MapBmpTo3dColors( dibMeta.m_hDib );				// LR_LOADMAP3DCOLORS doesn't work for images > 8bpp, we need to do the post-conversion

			dibMeta.StorePixelFormat();
		}

		return dibMeta;
	}


	COLORREF MapToSysColor( COLORREF color, bool useRGBQUAD );
	bool MapBmpTo3dColorsImpl( HBITMAP& rhBitmap, const BITMAP* pBmp, bool useRGBQUAD = false, COLORREF clrSrc = CLR_NONE, COLORREF clrDest = CLR_NONE );

	bool MapBmpTo3dColors( HBITMAP& rhBitmap, bool useRGBQUAD /*= false*/, COLORREF clrSrc /*= CLR_NONE*/, COLORREF clrDest /*= CLR_NONE*/ )
	{
		BITMAP bmp;
		if ( rhBitmap != NULL && ::GetObject( rhBitmap, sizeof( BITMAP ), &bmp ) != 0 )
			if ( bmp.bmBitsPixel > 8 )								// LR_LOADMAP3DCOLORS doesn't work for images > 8bpp, we should convert it now
				return MapBmpTo3dColorsImpl( rhBitmap, &bmp, useRGBQUAD, clrSrc, clrDest );

		return false;
	}

	bool MapBmpTo3dColorsImpl( HBITMAP& rhBitmap, const BITMAP* pBmp, bool useRGBQUAD /*= false*/, COLORREF clrSrc /*= CLR_NONE*/, COLORREF clrDest /*= CLR_NONE*/ )
	{
		ASSERT_PTR( rhBitmap );
		ASSERT_PTR( pBmp );
		ASSERT( CLR_NONE == clrSrc || clrDest != CLR_NONE );

		// create source memory DC and select an original bitmap
		CDC srcMemDC;
		srcMemDC.CreateCompatibleDC( NULL );

		HBITMAP hOldSrcBitmap = (HBITMAP)srcMemDC.SelectObject( rhBitmap );
		if ( NULL == hOldSrcBitmap )
			return false;

		// create a new bitmap compatible with the source memory DC (original bitmap should be already selected)
		HBITMAP hNewBitmap = (HBITMAP)::CreateCompatibleBitmap( srcMemDC, pBmp->bmWidth, pBmp->bmHeight );
		if ( NULL == hNewBitmap )
		{
			srcMemDC.SelectObject( hOldSrcBitmap );
			return false;
		}

		CDC destMemDC;				// create destination memory DC
		destMemDC.CreateCompatibleDC( &srcMemDC );

		HBITMAP hOldDestBitmap = (HBITMAP)destMemDC.SelectObject( hNewBitmap );
		if ( NULL == hOldDestBitmap )
		{
			srcMemDC.SelectObject( hOldSrcBitmap );
			::DeleteObject( hNewBitmap );
			return FALSE;
		}

		destMemDC.BitBlt( 0, 0, pBmp->bmWidth, pBmp->bmHeight, &srcMemDC, 0, 0, SRCCOPY );		// copy original bitmap to new

		// change a specific colors to system colors
		for ( int x = 0; x != pBmp->bmWidth; ++x )
			for ( int y = 0; y != pBmp->bmHeight; ++y )
			{
				COLORREF clrOrig = ::GetPixel( destMemDC, x, y );
				if ( clrSrc != CLR_NONE )
				{
					if ( clrOrig == clrSrc )
						::SetPixel( destMemDC, x, y, clrDest );
				}
				else
				{
					COLORREF clrNew = MapToSysColor( clrOrig, useRGBQUAD );

					if ( clrOrig != clrNew )
						::SetPixel( destMemDC, x, y, clrNew );
				}
			}

		destMemDC.SelectObject( hOldDestBitmap );
		srcMemDC.SelectObject( hOldSrcBitmap );

		::DeleteObject( rhBitmap );
		rhBitmap = hNewBitmap;
		return true;
	}


	#define AFX_RGB_TO_RGBQUAD( r, g, b )	( RGB( b, g, r ) )
	#define AFX_CLR_TO_RGBQUAD( clr )		( RGB( GetBValue( clr ), GetGValue( clr ), GetRValue( clr ) ) )

	COLORREF MapToSysColor( COLORREF color, bool useRGBQUAD )
	{
		struct COLORMAP
		{
			DWORD rgbqFrom;			// use DWORD instead of RGBQUAD so we can compare two RGBQUADs easily
			int toSysColor;
		};
		static const COLORMAP sysColorMap[] =
		{
			// mapping from color in DIB to system color
			{ AFX_RGB_TO_RGBQUAD( 0x00, 0x00, 0x00 ),  COLOR_BTNTEXT },			// black
			{ AFX_RGB_TO_RGBQUAD( 0x80, 0x80, 0x80 ),  COLOR_BTNSHADOW },		// dark grey
			{ AFX_RGB_TO_RGBQUAD( 0xC0, 0xC0, 0xC0 ),  COLOR_BTNFACE },			// bright grey
			{ AFX_RGB_TO_RGBQUAD( 0xFF, 0xFF, 0xFF ),  COLOR_BTNHIGHLIGHT }		// white
		};

		// look for matching RGBQUAD color in original
		for ( int i = 0; i != COUNT_OF( sysColorMap ); ++i )
			if ( color == sysColorMap[ i ].rgbqFrom )
				return useRGBQUAD
					? AFX_CLR_TO_RGBQUAD( afxGlobalData.GetColor( sysColorMap[ i ].toSysColor ) )
					: afxGlobalData.GetColor( sysColorMap[ i ].toSysColor );

		return color;
	}

} //namespace gdi
