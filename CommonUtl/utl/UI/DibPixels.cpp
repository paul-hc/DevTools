
#include "pch.h"
#include "DibPixels.h"
#include "DibSection.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDibPixels::CDibPixels( const CDibSection* pDibSection /*= nullptr*/ )
	: m_pDibSection( nullptr )
	, m_ownsDib( false )
	, m_pPixels( nullptr )
{
	if ( pDibSection != nullptr && pDibSection->IsValid() )
		Init( *pDibSection );
}

CDibPixels::CDibPixels( HBITMAP hDib, gdi::Orientation orientation /*= gdi::BottomUp*/ )
	: m_pDibSection( MakeLocalDibSection( hDib ) )			// create a local owned object that doesn't own the handle
	, m_ownsDib( true )
	, m_pPixels( nullptr )
	, m_orientation( orientation )							// a top-down DIB if height is negative
{
	CDibSectionTraits dibTraits( hDib );

	if ( m_pDibSection != nullptr && dibTraits.IsValid() )
	{
		REQUIRE( dibTraits.GetHeight() > 0 );				// GetObject should always returns positive

		m_pPixels = dibTraits.GetPixelBuffer();
		m_bufferSize = dibTraits.GetBufferSize();
		m_width = dibTraits.GetWidth();
		m_height = dibTraits.GetHeight();
		m_bitsPerPixel = dibTraits.GetBitsPerPixel();
		m_stride = dibTraits.GetStride();
		ENSURE( IsValid() );
	}
	else
		Reset();			// no pixel access for DDBs
}

CDibPixels::~CDibPixels()
{
	if ( m_ownsDib )
		Reset();
}

CDibSection* CDibPixels::MakeLocalDibSection( HBITMAP hDib )
{
	ASSERT_PTR( hDib );
	if ( is_a<CDibSection>( CGdiObject::FromHandle( hDib ) ) )
		return nullptr;				// should've called the overloaded constructor

	return new CDibSection( hDib );
}

void CDibPixels::Init( const CDibSection& dibSection )
{
	if ( IsValid() )
		Reset();

	m_pDibSection = const_cast<CDibSection*>( &dibSection );
	m_ownsDib = false;

	CDibSectionTraits dibTraits( dibSection.GetBitmapHandle() );
	ASSERT( dibTraits.IsValid() );

	m_pPixels = dibTraits.GetPixelBuffer();
	m_bufferSize = dibTraits.GetBufferSize();
	m_orientation = dibSection.GetSrcMeta().m_orientation;
	m_width = dibTraits.GetWidth();
	m_height = dibTraits.GetHeight();
	m_bitsPerPixel = dibTraits.GetBitsPerPixel();
	m_stride = dibTraits.GetStride();
}

void CDibPixels::Reset( void )
{
	if ( !IsValid() )
		return;

	if ( m_ownsDib && m_pDibSection != nullptr )
	{
		m_pDibSection->Detach();			// no ownership on the DIB handle
		delete m_pDibSection;
	}

	m_pDibSection = nullptr;
	m_ownsDib = false;

	m_pPixels = nullptr;
	m_bufferSize = 0;
	m_orientation = gdi::BottomUp;
	m_width = m_height = 0;
	m_bitsPerPixel = 0;
	m_stride = 0;
}

HBITMAP CDibPixels::GetBitmapHandle( void ) const implement
{
	return m_pDibSection != nullptr ? m_pDibSection->GetBitmapHandle() : nullptr;
}

bmp::CSharedAccess* CDibPixels::GetTarget( void ) const
{
	return m_pDibSection;
}

bool CDibPixels::FlipBottomUp( void )
{
	if ( m_orientation != gdi::BottomUp )
		return false;

	m_pPixels = m_pPixels + ( m_height - 1 ) * m_stride;
	m_stride = -m_stride;
	return true;
}

COLORREF* CDibPixels::GetTranspColorPtr( void )
{
	return m_pDibSection->HasTranspColor() ? &m_pDibSection->RefTranspColor() : nullptr;
}

bool CDibPixels::ContainsAlpha( void ) const
{
	typedef const CPixelBGRA* const_iterator;

	if ( HasAlpha() )
		for ( const_iterator pPixel = Begin<CPixelBGRA>(), pPixelEnd = End<CPixelBGRA>(); pPixel != pPixelEnd; ++pPixel )
			if ( pPixel->m_alpha != 0 )
				return true;

	return false;
}

bool CDibPixels::NeedsPreMultiplyAlpha( void ) const
{
	ASSERT( IsValid() );
	if ( 32 == m_bitsPerPixel )
	{
		typedef const CPixelBGRA* const_iterator;
		for ( const_iterator pPixel = Begin<CPixelBGRA>(), pPixelEnd = End<CPixelBGRA>(); pPixel != pPixelEnd; ++pPixel )
			if ( pPixel->NeedsPreMultiplyAlpha() )
				return true;
	}

	return false;
}

bool CDibPixels::PreMultiplyAlpha( bool autoCheckPMA /*= false*/ )
{
	ASSERT( IsValid() );
	if ( m_bitsPerPixel != 32 )
		return false;

	if ( autoCheckPMA )
		if ( !NeedsPreMultiplyAlpha() )
			return false;

	typedef CPixelBGRA* iterator;
	for ( iterator pPixel = Begin<CPixelBGRA>(), pPixelEnd = End<CPixelBGRA>(); pPixel != pPixelEnd; ++pPixel )
		pPixel->PreMultiplyAlpha();

	return true;
}

// Use this for copying between 24/32 bit DIBs.
// Use BitBlt for copying across indexed DIBs (src or dest): it's faster and does good color mapping.
// Avoid copying using GetPixelColor() or SetPixelColor(), it's unreliable and doesn't do color mapping.
// This has been tested thoroughly with all bit combinations. All assign methods involving GetPixelColor/SetPixelColor were deleted.
//
bool CDibPixels::CopyPixels( const CDibPixels& srcPixels )
{
	if ( m_bitsPerPixel == srcPixels.m_bitsPerPixel )
		switch ( m_bitsPerPixel )
		{
			case 32: Assign<CPixelBGRA, CPixelBGRA>( srcPixels, func::CopyPixel() ); break;
			case 24: Assign<CPixelBGR, CPixelBGR>( srcPixels, func::CopyPixel() ); break;
			case 16:
			default: CopyBuffer( srcPixels ); break;
		}
	else
	{
		switch ( m_bitsPerPixel )
		{
			case 32:
				if ( 24 == srcPixels.m_bitsPerPixel )
					Assign<CPixelBGRA, CPixelBGR>( srcPixels, func::CopyPixel() );
				else
					AssignSrcColor<CPixelBGRA>( srcPixels, func::CopyPixel() );
				break;
			case 24:
				if ( 32 == srcPixels.m_bitsPerPixel )
					Assign<CPixelBGR, CPixelBGRA>( srcPixels, func::CopyPixel() );
				else
					AssignSrcColor<CPixelBGR>( srcPixels, func::CopyPixel() );
				break;
			default:
				if ( 32 == srcPixels.m_bitsPerPixel )
					AssignDestColor<CPixelBGRA>( srcPixels );
				else if ( 24 == srcPixels.m_bitsPerPixel )
					AssignDestColor<CPixelBGR>( srcPixels );
				else
					return false;
				break;
		}
	}
	return true;
}

void CDibPixels::CopyBuffer( const CDibPixels& srcPixels )
{
	ASSERT( IsValid() && srcPixels.IsValid() );
	ASSERT( m_bufferSize == srcPixels.m_bufferSize );
	utl::Copy( srcPixels.m_pPixels, srcPixels.m_pPixels + srcPixels.m_bufferSize, m_pPixels );
}
