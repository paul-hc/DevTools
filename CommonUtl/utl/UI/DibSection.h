#ifndef DibSection_h
#define DibSection_h
#pragma once

#include "ScopedBitmapMemDC.h"
#include "Color.h"
#include "Image_fwd.h"


class CDibPixels;


// works with PNGs or DIB-BMPp 4/8/24/32bpp loaded from file or resource

class CDibSection : public CBitmap
	, public bmp::CSharedAccess
{
	enum DibFlags
	{
		F_IsPng				= 1 << 0,
		F_HasAlpha			= 1 << 1,
		F_AutoTranspColor	= 1 << 2,
		F_NoAutoDelete		= 1 << 5,
			_CopyMask = F_IsPng | F_HasAlpha | F_AutoTranspColor
	};
public:
	CDibSection( HBITMAP hDib = nullptr, bool ownsDib = false );
	CDibSection( const CDibSection* pSrcDib );
	virtual ~CDibSection();

	// base overrides
	virtual HBITMAP GetBitmapHandle( void ) const implement { return *this; }

	void Clear( void );
	bool AttachDib( const CDibMeta& dibMeta );

	// the preffered copy methods
	bool Copy( HBITMAP hSrcBitmap );
	bool Copy( const CDibSection* pSrcDib );								// preserve transparent color

	bool Convert( const CDibSection& srcDib, UINT destBitsPerPixel );
	bool CopyPixels( const CDibSection& srcDib, bool keepOrientation = false );		// via CDibPixels; pretty unreliable

	bool IsValid( void ) const { return GetSafeHandle() != nullptr; }
	bool IsDibSection( void ) const;
	bool IsIndexed( void ) const { ASSERT( IsDibSection() ); return m_bitsPerPixel <= 8; }
	const CDibMeta& GetSrcMeta( void ) const { return m_srcDibMeta; }		// source image information preserved

	WORD GetBitsPerPixel( void ) const { return m_bitsPerPixel; }
	const CSize& GetSize( void ) const { ASSERT( IsValid() && m_bitmapSize == gdi::GetBitmapSize( GetBitmapHandle() ) ); return m_bitmapSize; }

	bool IsPng( void ) const { return HasFlag( m_flags, F_IsPng ); }
	bool HasAlpha( void ) const { ASSERT( !HasFlag( m_flags, F_HasAlpha ) || m_bitsPerPixel >= 32 ); return HasFlag( m_flags, F_HasAlpha ); }
	bool HasTransparency( void ) const { return HasAlpha() || m_transpColor != CLR_NONE; }

	bool HasTranspColor( void ) const { return m_transpColor != CLR_NONE; }
	COLORREF GetTranspColor( void ) const { return m_transpColor; }
	void SetTranspColor( COLORREF transpColor );
	COLORREF& RefTranspColor( void ) { return m_transpColor; }

	bool HasAutoTranspColor( void ) const { return HasFlag( m_flags, F_AutoTranspColor ); }
	COLORREF FindAutoTranspColor( void ) const;
	bool SetAutoTranspColor( void );

	bool LoadFromFile( const TCHAR* pFilePath, ui::ImagingApi api = ui::WicApi, UINT framePos = 0 );		// framePos works only for WicApi

	bool LoadPngResource( UINT pngId, ui::ImagingApi api = ui::WicApi );
	bool LoadBitmapResource( UINT bitmapId );
	bool LoadImageResource( UINT imageId, ui::ImagingApi api = ui::WicApi );		// PNG or BMP

	ui::CImageListInfo MakeImageList( CImageList& rDestImageList, int imageCount, bool preserveThis = false );
	TImageListFlags CreateEmptyImageList( CImageList& rDestImageList, const CSize& imageSize, int imageCount ) const;		// bpp compatible with this DIB
	TImageListFlags GetImageListFlags( void ) const { return m_srcDibMeta.GetImageListFlags(); }

	// negative height for a top-down DIB (positive for bottpm-up DIB)
	bool CreateDIBSection( CDibPixels& rPixels, const BITMAPINFO& dibInfo );
	bool CreateDIBSection( CDibPixels& rPixels, int width, int height, UINT bitsPerPixel );

	// DIB/DDB drawing - caller must use a CScopedPalette if the DIB has got a palette
	// draws transparently if it's got an alpha channel or transparent color
	enum DrawResult { Error, AlphaBlended, TranspDrawn, Blitted };

	bool Blit( CDC* pDC, const CRect& rect, DWORD rop = SRCCOPY ) const;
	DrawResult DrawTransparent( CDC* pDC, const CRect& rect, BYTE srcAlpha = 255, BYTE blendOp = AC_SRC_OVER ) const;
	DrawResult Draw( CDC* pDC, const CRect& rect ) const;
private:
	bool CanDraw( void ) const { return IsValid() && m_bitmapSize.cx > 0 && m_bitmapSize.cy > 0; }
	COLORREF GetAutoFillColor( COLORREF fillColor = GetSysColor( COLOR_BTNFACE ) ) const;

	bool DoAlphaBlend( CDC* pDC, const CRect& rect, BYTE srcAlpha = 255, BYTE blendOp = AC_SRC_OVER ) const;
	bool DoBlit( CDC* pDC, const CRect& rect, DWORD rop = SRCCOPY ) const;
private:
	// hidden base members
	using CBitmap::Attach;
private:
	// hide base resource loading
	using CBitmap::LoadBitmap;
	using CBitmap::LoadOEMBitmap;
	using CBitmap::LoadMappedBitmap;
private:
	CDibMeta m_srcDibMeta;							// source image information preserved from creation
	int m_flags;
	WORD m_bitsPerPixel;
	CSize m_bitmapSize;
	COLORREF m_transpColor;
public:
	enum { ForceCvtEqualBpp = 1 << 0, ForceCvtCopyPixels = 1 << 1 };

	static CBitmap* s_pNullMask;
	static int s_testFlags;
};


#endif // DibSection_h
