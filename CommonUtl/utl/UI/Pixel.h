#ifndef Pixel_h
#define Pixel_h
#pragma once

#include "Color.h"


#pragma pack( push )
#pragma pack( 1 )				// 1 byte alignment for pixel structures


struct CPixelBGR				// the pixel color format for ILC_COLOR24: 0xrrggbb
{
	CPixelBGR( BYTE red, BYTE green, BYTE blue ) : m_blue( blue ), m_green( green ), m_red( red ) {}
	CPixelBGR( COLORREF color ) : m_blue( GetBValue( color ) ), m_green( GetGValue( color ) ), m_red( GetRValue( color ) ) {}
	CPixelBGR( const RGBQUAD& rgbQuad ) : m_blue( rgbQuad.rgbBlue ), m_green( rgbQuad.rgbGreen ), m_red( rgbQuad.rgbRed ) {}
	CPixelBGR( const PALETTEENTRY& palEntry ) : m_blue( palEntry.peBlue ), m_green( palEntry.peGreen ), m_red( palEntry.peRed ) {}

	COLORREF GetColor( void ) const { return RGB( m_red, m_green, m_blue ); }
public:
	BYTE m_blue, m_green, m_red;
};


struct CPixelBGRA				// the pixel color format for ILC_COLOR32: 0xaarrggbb
{
	CPixelBGRA( BYTE red, BYTE green, BYTE blue, BYTE alpha ) : m_blue( blue ), m_green( green ), m_red( red ), m_alpha( alpha ) {}
	CPixelBGRA( COLORREF color, BYTE alpha = 255 ) : m_blue( GetBValue( color ) ), m_green( GetGValue( color ) ), m_red( GetRValue( color ) ), m_alpha( alpha ) {}
	CPixelBGRA( RGBQUAD rgbQuad ) : m_blue( rgbQuad.rgbBlue ), m_green( rgbQuad.rgbGreen ), m_red( rgbQuad.rgbRed ), m_alpha( rgbQuad.rgbReserved ) {}

	COLORREF GetColor( void ) const { return RGB( m_red, m_green, m_blue ); }

	bool NeedsPreMultiplyAlpha( void ) const { return m_blue > m_alpha || m_green > m_alpha || m_red > m_alpha; }

	// pre-multiplied alpha (PMA) effect: ignore source RBG when A=0 (fully transparent);
	// this will get rid of the whiteness of transparent bk when ( 255, 255, 255, 0 )
	void PreMultiplyAlpha( void )
	{
		// premultiply the B, G, R values with the Alpha channel values
		PreMultiplyAlpha( m_blue );
		PreMultiplyAlpha( m_green );
		PreMultiplyAlpha( m_red );
	}

	void PreMultiplyAlpha( BYTE& rChannel ) const
	{
		rChannel = static_cast<BYTE>( rChannel * m_alpha / 255 );
	}
public:
	BYTE m_blue, m_green, m_red;
	BYTE m_alpha;						// opacity
};


#pragma pack( pop )


namespace pixel
{
	// color channel algorithms

	inline void FactorMultiplyChannel( BYTE& rChannel, double factor )
	{
		rChannel = static_cast<BYTE>( factor * rChannel );
	}

	template< typename ChannelType >
	inline void MultiplyChannel( ChannelType& rChannel, BYTE by )
	{
		rChannel = static_cast<ChannelType>( (UINT)rChannel * by / 255 );
	}

	inline BYTE GetMultiplyChannel( UINT channel, BYTE by )
	{
		return static_cast<BYTE>( channel * by / 255 );
	}

	inline BYTE GetAverageChannel( UINT channel1, UINT channel2 )
	{
		return static_cast<BYTE>( ( channel1 + channel2 ) / 2 );
	}

	inline void BlendChannel( BYTE& rFromChannel, BYTE toChannel, BYTE toAlpha )	// toAlpha=0 -> rFromChannel, toAlpha=255 -> toChannel
	{
		rFromChannel = static_cast<BYTE>( GetMultiplyChannel( rFromChannel, 255 - toAlpha ) + GetMultiplyChannel( toChannel, toAlpha ) );
	}


	// pixel algorithms

	template< typename PixelType >
	void SetColor( PixelType& rPixel, COLORREF color )
	{
		rPixel.m_blue = GetBValue( color );
		rPixel.m_green = GetGValue( color );
		rPixel.m_red = GetRValue( color );
	}

	template< typename PixelType >
	inline BYTE GetLuminance( const PixelType& rPixel )
	{
		return ui::GetLuminance( rPixel.m_red, rPixel.m_green, rPixel.m_blue );
	}

	template< typename PixelType >
	inline void ToGrayScale( PixelType& rPixel )
	{
		rPixel.m_blue = rPixel.m_green = rPixel.m_red = GetLuminance( rPixel );
	}

	template< typename PixelType >
	inline void BlendColor( PixelType& rPixel, const CPixelBGRA& toPixel )			// shifts the color towards toPixel using toPixel's alpha
	{
		BlendChannel( rPixel.m_blue, toPixel.m_blue, toPixel.m_alpha );
		BlendChannel( rPixel.m_green, toPixel.m_green, toPixel.m_alpha );
		BlendChannel( rPixel.m_red, toPixel.m_red, toPixel.m_alpha );
	}

	template<>
	inline void BlendColor<CPixelBGRA>( CPixelBGRA& rPixel, const CPixelBGRA& toPixel )	// shifts the color towards toPixel using toPixel's alpha
	{
		if ( rPixel.m_alpha != 0 )
		{
			BlendChannel( rPixel.m_blue, toPixel.m_blue, toPixel.m_alpha );
			BlendChannel( rPixel.m_green, toPixel.m_green, toPixel.m_alpha );
			BlendChannel( rPixel.m_red, toPixel.m_red, toPixel.m_alpha );
			rPixel.PreMultiplyAlpha();
		}
	}

	inline void AlphaChannelBlend( CPixelBGRA& rPixel, BYTE alpha )
	{
		rPixel.m_alpha = rPixel.m_alpha * alpha / 255;								// reduce only alpha (increase transparency)
	}

	template< typename PixelType >
	inline void MultiplyColors( PixelType& rPixel, double factor )
	{
		FactorMultiplyChannel( rPixel.m_blue, factor );
		FactorMultiplyChannel( rPixel.m_green, factor );
		FactorMultiplyChannel( rPixel.m_red, factor );
	}

	inline void MultiplyAlpha( CPixelBGRA& rPixel, BYTE alpha, double alphaFactor )
	{
		MultiplyColors( rPixel, alphaFactor );
		MultiplyChannel( rPixel.m_alpha, alpha );
	}
}


namespace func
{
	struct CopyPixel
	{
		void operator()( BYTE& rDestPixel, const BYTE& srcPixel ) const
		{
			rDestPixel = srcPixel;
		}

		void operator()( CPixelBGR& rDestPixel, const CPixelBGR& srcPixel ) const
		{
			rDestPixel = srcPixel;
		}

		void operator()( CPixelBGRA& rDestPixel, const CPixelBGRA& srcPixel ) const
		{
			// assume source is PMA, so no pre-multiplication needs to be done for destination
			if ( srcPixel.m_alpha != 0 )				// ignore transparent source pixels
				rDestPixel = srcPixel;
		}

		void operator()( CPixelBGRA& rDestPixel, const CPixelBGR& srcPixel ) const
		{
			rDestPixel.m_blue = srcPixel.m_blue;
			rDestPixel.m_green = srcPixel.m_green;
			rDestPixel.m_red = srcPixel.m_red;
			rDestPixel.m_alpha = 255;					// opaque
		}

		void operator()( CPixelBGR& rDestPixel, const CPixelBGRA& srcPixel ) const
		{
			rDestPixel.m_blue = srcPixel.m_blue;
			rDestPixel.m_green = srcPixel.m_green;
			rDestPixel.m_red = srcPixel.m_red;
		}
	};


	abstract class CBasePixelFunc			// color support for RGB pixel access: if the functor stores colours, adjust them to the DC nearest match
	{
	public:
		void AdjustColors( CDC* pDC ) { pDC; }
	protected:
		void AdjustColor( COLORREF& rColor, CDC* pDC )
		{
			rColor = ui::GetMappedColor( rColor, pDC );
		}

		template< typename PixelType >
		void AdjustPixel( PixelType& rPixel, CDC* pDC )
		{
			pixel::SetColor( rPixel, ui::GetMappedColor( rPixel.GetColor(), pDC ) );
		}
	};


	struct SetAlpha : public CBasePixelFunc				// aka set opacity
	{
		SetAlpha( BYTE alpha ) : m_alpha( alpha ) {}

		void operator()( CPixelBGR& rPixel ) const
		{
			rPixel;			// no-op; required for compilation of some algorithms
		}

		void operator()( CPixelBGRA& rPixel ) const
		{
			rPixel.m_alpha = m_alpha;
		}
	private:
		BYTE m_alpha;
	};


	struct SetColor : public CBasePixelFunc				// aka set opacity
	{
		SetColor( COLORREF color, BYTE alpha = 255 ) : m_pixel( color, alpha ) {}

		void operator()( CPixelBGR& rPixel ) const
		{
			rPixel.m_blue = m_pixel.m_blue;
			rPixel.m_green = m_pixel.m_green;
			rPixel.m_red = m_pixel.m_red;
		}

		void operator()( CPixelBGRA& rPixel ) const
		{
			rPixel = m_pixel;
		}
	private:
		CPixelBGRA m_pixel;
	};


	struct ReplaceColor : public CBasePixelFunc
	{
		ReplaceColor( COLORREF fromColor, COLORREF toColor, BYTE alpha = 255 ) : m_fromColor( fromColor ), m_toColor( toColor, alpha ) {}

		void operator()( CPixelBGR& rPixel ) const
		{
			if ( m_fromColor == rPixel.GetColor() )
			{
				rPixel.m_blue = m_toColor.m_blue;
				rPixel.m_green = m_toColor.m_green;
				rPixel.m_red = m_toColor.m_red;
			}
		}

		void operator()( CPixelBGRA& rPixel ) const
		{
			if ( m_fromColor == rPixel.GetColor() )			// compare colors, ignoring alpha
				rPixel = m_toColor;
			else
				rPixel.m_alpha = 255;						// make other pixels fully opaque
		}
	private:
		COLORREF m_fromColor;
		CPixelBGRA m_toColor;
	};


	/* Note 24 Nov, 2018:
		Something odd is happening with AlphaBlend, at least for 32bpp bitmaps - blending looks too "opaque" for my expectations.
		Because of that AlphaBlend doesn't seem very reliable.
	*/
	struct AlphaBlend : public CBasePixelFunc				// aka MultiplyAlpha
	{
		AlphaBlend( BYTE alpha, COLORREF blendColor24 = color::AzureBlue )
			: m_alpha( alpha ), m_alphaFactor( m_alpha / 255.0 ), m_blendColor24( blendColor24, 255 - m_alpha ) {}

		void operator()( CPixelBGR& rPixel ) const
		{
			pixel::BlendColor( rPixel, m_blendColor24 );			// effect very similar with 32 bpp
				//pixel::MultiplyColors( rPixel, m_alphaFactor );		// this doesn't work on 24 bit: it darkens the colors to black
		}

		void operator()( CPixelBGRA& rPixel ) const
		{
			pixel::MultiplyAlpha( rPixel, m_alpha, m_alphaFactor );		// this is the effect identical to alpha-blending in 32 bpp
		}

		void AdjustColors( CDC* pDC ) { AdjustPixel( m_blendColor24, pDC ); }
	private:
		BYTE m_alpha;
		double m_alphaFactor;
		CPixelBGRA m_blendColor24;
	};


	struct BlendColor : public CBasePixelFunc
	{
		BlendColor( COLORREF toColor24, BYTE toAlpha ) : m_toPixel( toColor24, toAlpha ) {}

		template< typename PixelType >
		void operator()( PixelType& rPixel ) const
		{
			pixel::BlendColor( rPixel, m_toPixel );
		}

		void AdjustColors( CDC* pDC ) { AdjustPixel( m_toPixel, pDC ); }
	private:
		CPixelBGRA m_toPixel;
	};


	struct ToGrayScale : public CBasePixelFunc
	{
		ToGrayScale( COLORREF transpColor24 = CLR_NONE ) : m_transpColor24( transpColor24 ) {}

		void operator()( CPixelBGR& rPixel ) const
		{
			if ( CLR_NONE == m_transpColor24 || rPixel.GetColor() != m_transpColor24 )
				pixel::ToGrayScale( rPixel );
		}

		void operator()( CPixelBGRA& rPixel ) const
		{
			pixel::ToGrayScale( rPixel );
		}

		void AdjustColors( CDC* pDC ) { AdjustColor( m_transpColor24, pDC ); }
	private:
		COLORREF m_transpColor24;
	};


	struct DisabledGrayOut : public CBasePixelFunc		// dim colors and make gray-scale
	{
		DisabledGrayOut( COLORREF toColor24, BYTE toAlpha ) : m_blendFunc( toColor24, toAlpha ), m_grayScaleFunc( CLR_NONE /*toColor24*/ ) {}

		template< typename PixelType >
		void operator()( PixelType& rPixel ) const
		{
			m_blendFunc( rPixel );
			m_grayScaleFunc( rPixel );
		}

		void AdjustColors( CDC* pDC )
		{
			m_blendFunc.AdjustColors( pDC );
			m_grayScaleFunc.AdjustColors( pDC );
		}
	private:
		BlendColor m_blendFunc;
		ToGrayScale m_grayScaleFunc;
	};


	struct DisabledEffect : public CBasePixelFunc			// dim colors towards toColor24
	{
		DisabledEffect( COLORREF toColor24, BYTE toAlpha ) : m_alphaBlendFunc( toAlpha, toColor24 ) {}

		template< typename PixelType >
		void operator()( PixelType& rPixel ) const
		{
			m_alphaBlendFunc( rPixel );
		}

		void AdjustColors( CDC* pDC ) { m_alphaBlendFunc.AdjustColors( pDC ); }
	private:
		AlphaBlend m_alphaBlendFunc;
	};


	struct _DisabledGrayEffect : public CBasePixelFunc		// dim colors and make gray-scale; made obsolete since AlphaBlend doesn't behave "naturally".
	{
		_DisabledGrayEffect( COLORREF toColor24, BYTE toAlpha ) : m_alphaBlendFunc( toAlpha, toColor24 ), m_grayScaleFunc( CLR_NONE /*toColor24*/ ) {}

		template< typename PixelType >
		void operator()( PixelType& rPixel ) const
		{
			m_grayScaleFunc( rPixel );
			m_alphaBlendFunc( rPixel );
		}

		void AdjustColors( CDC* pDC )
		{
			m_grayScaleFunc.AdjustColors( pDC );
			m_alphaBlendFunc.AdjustColors( pDC );
		}
	private:
		AlphaBlend m_alphaBlendFunc;
		ToGrayScale m_grayScaleFunc;
	};
}


#endif // Pixel_h
