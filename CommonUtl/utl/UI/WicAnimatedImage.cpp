
#include "pch.h"
#include "WicAnimatedImage.h"
#include "ImagingDirect2D.h"
#include "ComUtils.h"
#include "GdiCoords.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const D2D1_COLOR_F CWicAnimatedImage::s_transparentColor = D2D1::ColorF( D2D1::ColorF::Black, 0.f );

CWicAnimatedImage::CWicAnimatedImage( const wic::CBitmapDecoder& decoder )
	: CWicImage( &GUID_WICPixelFormat32bppPBGRA )
	, m_decoder( decoder )
	, m_gifSize( 0, 0 )
	, m_gifPixelSize( 0, 0 )
	, m_bkgndColor( s_transparentColor )
	, m_isGif( m_decoder.HasContainerFormat( GUID_ContainerFormatGif ) )
{
	ASSERT( m_decoder.IsValid() );
	StoreGlobalMetadata();
}

CWicAnimatedImage::~CWicAnimatedImage()
{
}

bool CWicAnimatedImage::IsAnimated( void ) const
{
	return true;
}

const CSize& CWicAnimatedImage::GetBmpSize( void ) const
{
	return m_gifPixelSize;
}

bool CWicAnimatedImage::StoreGlobalMetadata( void )
{
	CComPtr<IWICMetadataQueryReader> pDecoderMetadata = m_decoder.GetDecoderMetadata();
	if ( nullptr == pDecoderMetadata )
		return false;

	com::CPropVariant prop;

	if ( SUCCEEDED( pDecoderMetadata->GetMetadataByName( L"/logscrdesc/Width", &prop ) ) )
		prop.GetInt( &m_gifSize.cx );

	if ( SUCCEEDED( pDecoderMetadata->GetMetadataByName( L"/logscrdesc/Height", &prop ) ) )
		prop.GetInt( &m_gifSize.cy );

	UINT pixelAspectRatio = 0;
	if ( SUCCEEDED( pDecoderMetadata->GetMetadataByName( L"/logscrdesc/PixelAspectRatio", &prop ) ) )
		prop.GetUInt( &pixelAspectRatio );

	if ( pixelAspectRatio != 0 )
	{
		// need to calculate the ratio: the value in pixelAspectRatio allows specifying widest pixel 4:1 to the tallest pixel of 1:4 in increments of 1/64th
		float pixelAspectRatioF = ( pixelAspectRatio + 15.f ) / 64.f;

		// calculate the image width and height in pixel based on the pixel aspect ratio - only shrink the image
		if ( pixelAspectRatioF > 1.f )
		{
			m_gifPixelSize.cx = m_gifSize.cx;
			m_gifPixelSize.cy = static_cast<long>( m_gifSize.cy / pixelAspectRatioF );
		}
		else
		{
			m_gifPixelSize.cx = static_cast<long>( m_gifSize.cx * pixelAspectRatioF );
			m_gifPixelSize.cy = m_gifSize.cy;
		}
	}
	else
		m_gifPixelSize = m_gifSize;		// the value is 0, so its ratio is 1

	if ( ui::IsEmptySize( m_gifSize ) )
		m_gifSize = m_gifPixelSize = GetBmpFmt().m_size;			// use first frame size if metadata is missing

	StoreBkgndColor( pDecoderMetadata );
	return true;
}

bool CWicAnimatedImage::StoreBkgndColor( IWICMetadataQueryReader* pDecoderMetadata )
{
	// store background color from the decoder metadata
	ASSERT_PTR( pDecoderMetadata );

	com::CPropVariant value;

	// if we have a global palette, get the palette and background color
	bool hasColorTable = false;
	if ( SUCCEEDED( pDecoderMetadata->GetMetadataByName( L"/logscrdesc/GlobalColorTableFlag", &value ) ) )
		value.GetBool( &hasColorTable );

	if ( !hasColorTable )
		return false;

	UINT bkgndIndex = 0;			// background color index
	if ( SUCCEEDED( pDecoderMetadata->GetMetadataByName( L"/logscrdesc/BackgroundColorIndex", &value ) ) )
		value.GetUInt( &bkgndIndex );

	// get the color from the palette
	CComPtr<IWICPalette> pPalette;
	if ( SUCCEEDED( wic::CImagingFactory::Factory()->CreatePalette( &pPalette ) ) )
		if ( SUCCEEDED( m_decoder.GetDecoder()->CopyPalette( pPalette ) ) )
		{
			std::vector<WICColor> colors( 256 );
			UINT colorCount = 0;
			if ( SUCCEEDED( pPalette->GetColors( static_cast<UINT>( colors.size() ), &colors.front(), &colorCount ) ) )
			{
				colors.resize( colorCount );
				if ( bkgndIndex < colors.size() )					// valid background color index?
				{
					WICColor bkgndColor = colors[ bkgndIndex ];		// ARGB format
					float alpha = d2d::GetAlpha( bkgndColor );
					m_bkgndColor = D2D1::ColorF( bkgndColor, alpha );
					return true;
				}
			}
		}

	return false;
}


// CWicAnimatedImage::CFrameMetadata implementation

void CWicAnimatedImage::CFrameMetadata::Reset( void )
{
	m_rect.SetRect( 0, 0, 0, 0 );
	m_frameDelay = NoDelay;
	m_frameDisposal = DM_None;
}

void CWicAnimatedImage::CFrameMetadata::Store( IWICMetadataQueryReader* pMetadataReader )
{
	// store the Metadata for the current frame
	com::CPropVariant value;

	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/imgdesc/Left", &value ) ) )
		value.GetInt( &m_rect.left );

	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/imgdesc/Top", &value ) ) )
		value.GetInt( &m_rect.top );

	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/imgdesc/Width", &value ) ) )
		if ( value.GetInt( &m_rect.right ) )
			m_rect.right += m_rect.left;

	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/imgdesc/Height", &value ) ) )
		if ( value.GetInt( &m_rect.bottom ) )
			m_rect.bottom += m_rect.top;

	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/grctlext/Delay", &value ) ) )
		if ( value.GetInt( &m_frameDelay ) )
		{
			m_frameDelay *= 10;				// convert from 10 ms units to 1 ms units
			m_frameDelay = std::max( (UINT)MinDelay, m_frameDelay );		// clamp up to ensure rendering for gif with very small or 0 delay
		}
		else
			m_frameDelay = NoDelay;			// possibly a non-animated gif (single frame image)

	m_frameDisposal = DM_Undefined;			// reset to default disposal method, assuming a non-animated gif
	if ( SUCCEEDED( pMetadataReader->GetMetadataByName( L"/grctlext/Disposal", &value ) ) )
		value.GetInt( &m_frameDisposal );
}
