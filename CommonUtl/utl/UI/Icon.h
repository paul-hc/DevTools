#ifndef Icon_h
#define Icon_h
#pragma once

#include "Image_fwd.h"
#include "Color.h"


namespace res { struct CGroupIconEntry; }

class CIconGroup;


class CIcon : private utl::noncopyable
{
public:
	CIcon( void );
	CIcon( HICON hIcon, const CSize& iconSize = CSize( 0, 0 ), TBitsPerPixel bitsPerPixel = 0 );
	CIcon( HICON hIcon, const res::CGroupIconEntry& groupIconEntry );
	CIcon( const CIcon& shared );
	~CIcon();

	static CIcon* LoadNewIcon( HICON hIcon );
	static CIcon* LoadNewIcon( const CIconId& iconId );
	static CIcon* LoadNewIcon( const res::CGroupIconEntry& groupIconEntry );	// load from resources the exact icon frame
	static CIcon* LoadNewExactIcon( const CIconId& iconId );					// loads only if size exists (no scaling)

	bool IsValid( void ) const { return m_hIcon != nullptr; }

	HICON GetSafeHandle( void ) const { return this != nullptr ? m_hIcon : nullptr; }

	HICON GetHandle( void ) const { return m_hIcon; }
	CIcon& SetHandle( HICON hIcon, bool hasAlpha );

	bool HasAlpha( void ) const { return IsValid() && m_hasAlpha; }			// based on a 32 bit per pixel image
	void SetHasAlpha( bool hasAlpha );
	static CIcon* SetHasAlpha( CIcon* pIcon, bool hasAlpha );				// safe if nullptr pIcon

	bool IsShared( void ) const { return m_pShared != nullptr; }
	CIcon* Clone( void ) const { return new CIcon( *this ); }
	void SetShared( const CIcon& shared );

	HICON Detach( void );
	void Clear( void );

	const CSize& GetSize( void ) const;
	int GetDimension( void ) const { GetSize(); return std::max( m_size.cx, m_size.cy ); }
	TBitsPerPixel GetBitsPerPixel( void ) const { return m_bitsPerPixel; }

	void Draw( HDC hDC, const CPoint& pos, bool enabled = true ) const;
	void DrawDisabled( HDC hDC, const CPoint& pos ) const;

	void DrawStretch( HDC hDC, const CRect& destRect, bool enabled = true ) const;

	// icon to bitmap
	bool MakeBitmap( CBitmap& rBitmap, COLORREF transpColor ) const;

	// icon from bitmap
	void CreateFromBitmap( HBITMAP hImageBitmap, HBITMAP hMaskBitmap );
	void CreateFromBitmap( HBITMAP hImageBitmap, COLORREF transpColor );

	static const CIcon& GetUnknownIcon( void );		// IDI_UNKNOWN missing icon placeholder
public:
	void ResetIconGroup( CIconGroup* pIconGroup = nullptr, size_t groupFramePos = utl::npos );

	bool HasIconGroup( void ) const { return m_pIconGroup != nullptr; }
	CIconGroup* GetIconGroup( void ) const { return m_pIconGroup; }
	size_t GetGroupFramePos( void ) const { return m_groupFramePos; }
	ui::CIconEntry GetGroupIconEntry( void ) const;
private:
	bool LazyComputeInfo( void ) const;
private:
	HICON m_hIcon;
	const CIcon* m_pShared;			// if shared it has no ownership (no ref counting, m_pShared must be kept alive - usually in the shared image store)
	bool m_hasAlpha;				// based on a 32 bit per pixel image; need to store this since both colour and mask bitmaps are DDBs

	mutable CSize m_size;
	TBitsPerPixel m_bitsPerPixel;

	CIconGroup* m_pIconGroup;
	size_t m_groupFramePos;
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
