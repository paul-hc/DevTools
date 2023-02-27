#ifndef Icon_h
#define Icon_h
#pragma once

#include "Image_fwd.h"
#include "Color.h"


class CIcon : private utl::noncopyable
{
public:
	CIcon( void );
	CIcon( HICON hIcon, const CSize& iconSize = CSize( 0, 0 ) );
	CIcon( const CIcon& shared );
	~CIcon();

	static CIcon* NewIcon( HICON hIcon );
	static CIcon* NewIcon( const CIconId& iconId );
	static CIcon* NewExactIcon( const CIconId& iconId );					// loads only if size exists (no scaling)

	bool IsValid( void ) const { return m_hIcon != nullptr; }

	HICON GetSafeHandle( void ) const { return this != nullptr ? m_hIcon : nullptr; }

	HICON GetHandle( void ) const { return m_hIcon; }
	CIcon& SetHandle( HICON hIcon, bool hasAlpha );

	bool HasAlpha( void ) const { return IsValid() && m_hasAlpha; }			// based on a 32 bit per pixel image
	void SetHasAlpha( bool hasAlpha ) { m_hasAlpha = hasAlpha; }
	static CIcon* SetHasAlpha( CIcon* pIcon, bool hasAlpha );				// safe if NULL pIcon

	bool IsShared( void ) const { return m_pShared != nullptr; }
	CIcon* Clone( void ) const { return new CIcon( *this ); }
	void SetShared( const CIcon& shared );

	HICON Detach( void );
	void Release( void );

	const CSize& GetSize( void ) const;

	void Draw( HDC hDC, const CPoint& pos, bool enabled = true ) const;
	void DrawDisabled( HDC hDC, const CPoint& pos ) const;

	// icon to bitmap
	bool MakeBitmap( CBitmap& rBitmap, COLORREF transpColor ) const;

	// icon from bitmap
	void CreateFromBitmap( HBITMAP hImageBitmap, HBITMAP hMaskBitmap );
	void CreateFromBitmap( HBITMAP hImageBitmap, COLORREF transpColor );

	static CSize ComputeSize( HICON hIcon );
	static const CIcon& GetUnknownIcon( void );		// IDI_UNKNOWN missing icon placeholder
private:
	HICON m_hIcon;
	bool m_hasAlpha;				// based on a 32 bit per pixel image; need to store this since both colour and mask bitmaps are DDBs
	const CIcon* m_pShared;			// if shared it has no ownership (no ref counting, m_pShared must be kept alive - usually in the shared image store)
	mutable CSize m_size;
};


struct CIconInfo : private utl::noncopyable
{
	CIconInfo( HICON hIcon, bool isCursor = false );
	~CIconInfo();

	bool IsValid( void ) const { return m_info.hbmColor != nullptr || m_info.hbmMask != nullptr; }	// if monochrome it has only mask
	bool IsCursor( void ) const { return !m_info.fIcon; }
	bool HasAlpha( void ) const { return IsValid() && nullptr == m_info.hbmMask; }

	CPoint GetHotSpot( void ) const { return CPoint( m_info.xHotspot, m_info.yHotspot ); }
	bool MakeDibSection( CBitmap& rDibSection ) const;			// uses hbmColor (ignores hbmMask)
private:
	void Init( HICON hIcon, bool isCursor );
public:
	ICONINFO m_info;			// colour and mask bitmaps are DDBs
	CSize m_size;

    // DDB bitmaps with ownership (to prevent leaks)
    CBitmap m_bitmapColor;
    CBitmap m_bitmapMask;
};


#endif // Icon_h
