#ifndef DibPixels_h
#define DibPixels_h
#pragma once

#include "ScopedBitmapMemDC.h"
#include "Image_fwd.h"
#include "Pixel.h"


class CDibSection;


class CDibPixels : protected bmp::CSharedAccess
{
public:
	CDibPixels( const CDibSection* pDibSection = nullptr );
	CDibPixels( HBITMAP hDib, gdi::Orientation orientation = gdi::BottomUp );
	~CDibPixels();

	void Init( const CDibSection& dibSection );

	const CDibSection* GetDib( void ) const { return m_pDibSection; }
	virtual HBITMAP GetHandle( void ) const;

	bool IsValid( void ) const { ASSERT( ( m_pDibSection == nullptr ) == ( m_pPixels == nullptr ) ); return m_pPixels != nullptr; }
	bool IsIndexed( void ) const { ASSERT( IsValid() ); return m_bitsPerPixel <= 8; }
	gdi::Orientation GetOrientation( void ) const { ASSERT( IsValid() ); return m_orientation; }
	bool HasAlpha( void ) const { return 32 == m_bitsPerPixel; }

	WORD GetBitsPerPixel( void ) const { return m_bitsPerPixel; }

	UINT GetWidth( void ) const { return m_width; }
	UINT GetHeight( void ) const { return m_height; }
	CSize GetBitmapSize( void ) const { return CSize( m_width, m_height ); }

	UINT GetSize( void ) const { return m_bufferSize; }
	UINT GetStride( void ) const { return m_stride; }

	// 32/24 bpp style iteration (BGRA, BGR and BYTE)

	template< typename PixelType > PixelType* Begin( void ) { ASSERT( ValidType<PixelType>() ); return reinterpret_cast<PixelType*>( m_pPixels ); }
	template< typename PixelType > PixelType* End( void ) { ASSERT( ValidLinearIteration<PixelType>() ); return reinterpret_cast<PixelType*>( m_pPixels + m_bufferSize ); }

	template< typename PixelType > const PixelType* Begin( void ) const { return const_cast<CDibPixels*>( this )->Begin<PixelType>(); }
	template< typename PixelType > const PixelType* End( void ) const { return const_cast<CDibPixels*>( this )->End<PixelType>(); }

	template< typename PixelType >
	PixelType& GetPixel( UINT x, UINT y )
	{
		ASSERT( sizeof( PixelType ) == ( m_bitsPerPixel / 8 ) || sizeof( PixelType ) == sizeof( BYTE ) );
		return *static_cast<PixelType*>( GetPixelPtr( x, y ) );
	}

	template< typename PixelType > const PixelType& GetPixel( UINT x, UINT y ) const { return const_cast<CDibPixels*>( this )->GetPixel<PixelType>( x, y ); }

	template< typename PixelType, typename PixelFunc > void ForEach( PixelFunc func );
	template< typename PixelFunc > bool ForEach( PixelFunc func );

	template< typename PixelFunc > void MapTranspColor( PixelFunc func );


	// 16/8/4/1 bpp style iteration (RGB with DC access)
	COLORREF GetPixelColor( UINT x, UINT y ) const
	{
		ASSERT( x < m_width && y < m_height );
		return GetBitmapMemDC()->GetPixel( x, y );
	}

	COLORREF SetPixelColor( UINT x, UINT y, COLORREF color )
	{
		ASSERT( x < m_width && y < m_height );
		return GetBitmapMemDC()->SetPixel( x, y, color );
	}


	// pixel algorithms
	bool ContainsAlpha( void ) const;
	bool NeedsPreMultiplyAlpha( void ) const;
	bool PreMultiplyAlpha( bool autoCheckPMA = false );
	bool CopyPixels( const CDibPixels& srcPixels );				// mixed results
	void CopyBuffer( const CDibPixels& srcPixels );				// for identical formats

	template< typename DestPixelType, typename SrcPixelType >
	void CopyRect( const CDibPixels& srcPixels, const CRect& rect, const CPoint& srcPos );

	void SetAlpha( BYTE alpha ) { ForEach<CPixelBGRA>( func::SetAlpha( alpha ) ); }
	void SetOpaque( void ) { SetAlpha( 255 ); }
	void Fill( COLORREF color, BYTE alpha = 255 ) { ForEach( func::SetColor( color, alpha ) ); }

	// conversion effects
	bool ApplyGrayScale( COLORREF transpColor24 = CLR_NONE ) { return ForEach( func::ToGrayScale( transpColor24 ) ); }
	bool ApplyAlphaBlend( BYTE alpha, COLORREF blendColor24 = color::AzureBlue ) { return ForEach( func::AlphaBlend( alpha, blendColor24 ) ); }
	bool ApplyBlendColor( COLORREF toColor24, BYTE toAlpha ) { return ForEach( func::BlendColor( toColor24, toAlpha ) ); }

	bool ApplyDisabledGrayOut( COLORREF toColor24, BYTE toAlpha = 64 ) { return ForEach( func::DisabledGrayOut( toColor24, toAlpha ) ); }
	bool ApplyDisabledEffect( COLORREF toColor24, BYTE toAlpha = 64 ) { return ForEach( func::DisabledEffect( toColor24, toAlpha ) ); }
	bool ApplyDisabledGrayEffect_( COLORREF toColor24, BYTE toAlpha = 64 ) { return ForEach( func::_DisabledGrayEffect( toColor24, toAlpha ) ); }
private:
	static CDibSection* MakeLocalDibSection( HBITMAP hDib );
	void Reset( void );
	COLORREF* GetTranspColorPtr( void );

	// base overrides
	virtual bmp::CSharedAccess* GetTarget( void ) const;

	UINT GetPixelY( UINT y ) const { return gdi::BottomUp == m_orientation ? y : ( m_height - y - 1 ); }		// aware of top-down-ness
	bool FlipBottomUp( void );		// not used, verbatim from CImage::UpdateBitmapInfo()

	template< typename PixelType > bool ValidType( void ) const { return IsValid() && sizeof( PixelType ) == ( m_bitsPerPixel / 8 ); }
	template<> bool ValidType<BYTE>( void ) const { return IsValid(); }

	template< typename PixelType > bool ValidLinearIteration( void ) const { return 32 == m_bitsPerPixel && ValidType<PixelType>(); }
	template<> bool ValidLinearIteration<BYTE>( void ) const { return ValidType<BYTE>(); }		// BYTE arithmetic works for 24bpp

	void* GetPixelPtr( UINT x, UINT y )
	{
		ASSERT( IsValid() );
		ASSERT( x < m_width && y < m_height );
		return m_pPixels + GetPixelY( y ) * m_stride + ( x * m_bitsPerPixel ) / 8;				// aware of top-down-ness
	}

	template< typename PixelType, typename PixelFunc > void ForEachXY( PixelFunc func );		// safe x,y iteration for 24bpp, aware of top-down-ness
	template< typename PixelFunc > void ForEachRGB( PixelFunc func );							// for 1/4/8/16 bpp; func takes CPixelBGR

	// aware of src/dest top-down-ness
	template< typename DestPixelType, typename SrcPixelType, typename AssignFunc > void Assign( const CDibPixels& srcPixels, AssignFunc func );
	template< typename DestPixelType, typename AssignFunc > void AssignSrcColor( const CDibPixels& srcPixels, AssignFunc func );
	template< typename SrcPixelType > void AssignDestColor( const CDibPixels& srcPixels );
private:
	CDibSection* m_pDibSection;
	bool m_ownsDib;						// owns temporary object when built on a HBITMAP

	BYTE* m_pPixels;
	UINT m_bufferSize;					// total byte count (all pixels)
	gdi::Orientation m_orientation;		// known at creation time in BITMAPINFO, must be passed from creation (can't be retrofitted)
	UINT m_width;
	UINT m_height;
	WORD m_bitsPerPixel;
	int m_stride;						// number of bytes per scan line (aka pitch)
};


// CDibPixels template code

template< typename PixelFunc >
void CDibPixels::MapTranspColor( PixelFunc func )
{
	if ( COLORREF* pTranspColor = GetTranspColorPtr() )
	{
		CPixelBGR pixel( *pTranspColor );
		func( pixel );
		*pTranspColor = pixel.GetColor();
	}
}

template< typename PixelType, typename PixelFunc >
void CDibPixels::ForEachXY( PixelFunc func )			// safer iteration
{
	ASSERT( ValidType<PixelType>() );

	for ( UINT y = 0; y != m_height; ++y )
		for ( UINT x = 0; x != m_width; ++x )
			func( GetPixel<PixelType>( x, y ) );

	MapTranspColor( func );
}

template< typename PixelFunc >
void CDibPixels::ForEachRGB( PixelFunc func )
{
	ASSERT( HasValidBitmapMemDC() );

	func.AdjustColors( GetBitmapMemDC() );				// functors must inherit from func::CBasePixelFunc

	for ( UINT y = 0; y != m_height; ++y )
		for ( UINT x = 0; x != m_width; ++x )
		{	// translate COLORREF to CPixelBGR and back
			CPixelBGR pixel( GetPixelColor( x, y ) );
			func( pixel );
			SetPixelColor( x, y, pixel.GetColor() );
		}

	MapTranspColor( func );
}

template< typename PixelType, typename PixelFunc >
void CDibPixels::ForEach( PixelFunc func )
{
	if ( !ValidLinearIteration<PixelType>() )
	{
		ForEachXY<PixelType>( func );		// requires x,y iteration
		return;
	}

	UINT pos = 0;

	try
	{	// faster linear iteration
		typedef PixelType* iterator;
		for ( iterator pPixel = Begin<PixelType>(), pPixelEnd = End<PixelType>(); pPixel != pPixelEnd; ++pPixel, ++pos )
			func( *pPixel );

		return;
	}
	catch ( ... )
	{
		TRACE( _T(" ** Pixel access failure at pos=%u; size=%u width=%u height=%u bpp=%u\n"), pos, m_bufferSize, m_width, m_height, m_bitsPerPixel );
	}
	ASSERT( false );	// memory access violation
}

template< typename PixelFunc >
bool CDibPixels::ForEach( PixelFunc func )
{
	switch ( m_bitsPerPixel )
	{
		case 32: ForEach<CPixelBGRA>( func ); return true;
		case 24: ForEach<CPixelBGR>( func ); return true;
	}

	CScopedBitmapMemDC scopedPixelAccess( this );
	if ( !HasValidBitmapMemDC() )
		return false;

	if ( IsIndexed() )
	{
		MapTranspColor( func );

		CDibSectionInfo dibInfo( GetHandle() );
		return dibInfo.ForEachInColorTable( *GetTarget(), func );		// just modify the color table
	}
	else
		ForEachRGB( func );										// 16 bit: apply CPixelBGR algorithms to RGB bitmaps
	return true;
}

template< typename DestPixelType, typename SrcPixelType, typename AssignFunc >
void CDibPixels::Assign( const CDibPixels& srcPixels, AssignFunc func )
{
	ASSERT( GetBitmapSize() == srcPixels.GetBitmapSize() );
	ASSERT( ValidType<DestPixelType>() );
	ASSERT( srcPixels.ValidType<SrcPixelType>() );

	for ( UINT y = 0; y != m_height; ++y )
		for ( UINT x = 0; x != m_width; ++x )
			func( GetPixel<DestPixelType>( x, y ), srcPixels.GetPixel<SrcPixelType>( x, y ) );
}

template< typename DestPixelType, typename AssignFunc >
void CDibPixels::AssignSrcColor( const CDibPixels& srcPixels, AssignFunc func )
{
	ASSERT( GetBitmapSize() == srcPixels.GetBitmapSize() );
	ASSERT( ValidType<DestPixelType>() );

	CScopedBitmapMemDC scopedSrcBitmap( &srcPixels );

	for ( UINT y = 0; y != m_height; ++y )
		for ( UINT x = 0; x != m_width; ++x )
			func( GetPixel<DestPixelType>( x, y ), CPixelBGR( srcPixels.GetPixelColor( x, y ) ) );
}

template< typename SrcPixelType >
void CDibPixels::AssignDestColor( const CDibPixels& srcPixels )
{
	ASSERT( GetBitmapSize() == srcPixels.GetBitmapSize() );
	ASSERT( srcPixels.ValidType<SrcPixelType>() );

	CScopedBitmapMemDC scopedBitmap( this );

	for ( UINT y = 0; y != m_height; ++y )
		for ( UINT x = 0; x != m_width; ++x )
			SetPixelColor( x, y, srcPixels.GetPixel<SrcPixelType>( x, y ).GetColor() );
}

template< typename DestPixelType, typename SrcPixelType >
void CDibPixels::CopyRect( const CDibPixels& srcPixels, const CRect& rect, const CPoint& srcPos )
{
	ASSERT( ValidType<DestPixelType>() );
	ASSERT( srcPixels.ValidType<SrcPixelType>() );

	func::CopyPixel copyFunc;
	UINT width = rect.Width(), height = rect.Height();

	for ( UINT y = 0; y != height; ++y )
		for ( UINT x = 0; x != width; ++x )
			copyFunc( GetPixel<DestPixelType>( rect.left + x, rect.top + y ), srcPixels.GetPixel<SrcPixelType>( srcPos.x + x, srcPos.y + y ) );
}


#endif // DibPixels_h
