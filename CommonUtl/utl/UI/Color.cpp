
#include "pch.h"
#include "Color.h"
#include "ColorRepository.h"
#include "Image_fwd.h"
#include "StringUtilities.h"
#include "utl/Algorithms.h"
#include "utl/TextClipboard.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	COLORREF EvalColor( COLORREF rawColor )
	{
		if ( rawColor != CLR_NONE && rawColor != CLR_DEFAULT )
			if ( IsSysColor( rawColor ) )
				return ::GetSysColor( GetSysColorIndex( rawColor ) );
			else
				ASSERT( IsRealColor( rawColor ) );		// physical color?

		return rawColor;
	}
}


namespace ui
{
	BYTE GetLuminance( UINT red, UINT green, UINT blue )
	{	// perceived luminance (gray level) of a color (Y component) = 0.3*R + 0.59*G + 0.11*B
		UINT luminance;
		if ( red == green && red == blue )
			luminance = red;					// preserve grays unchanged
		else
			luminance = ( red * 30 + green * 59 + blue * 11 ) / 100;

		ASSERT( luminance <= 255 );
		return static_cast<BYTE>( luminance );
	}
}


namespace color
{
	const std::tstring g_nullTag = _T("(null)");
	const std::tstring g_autoTag = _T("(Automatic)");
	const std::tstring g_sysTag = _T("SYS");
	const std::tstring g_rgbTag = _T("RGB");
	const std::tstring g_htmlTag = _T("#");
	const std::tstring g_hexTag = _T("0x");
}


namespace ui
{
	std::tstring FormatColor( COLORREF color, const TCHAR* pSep /*= _T("  ")*/, bool inColorEntry /*= false*/ )
	{
		if ( CLR_NONE == color )
			return color::g_nullTag;
		else if ( CLR_DEFAULT == color )
			return color::g_autoTag;

		std::tstring colorLiteral;
		colorLiteral.reserve( 24 );

		if ( IsSysColor( color ) )
			stream::Tag( colorLiteral, FormatSysColor( color, inColorEntry ), pSep );

		const COLORREF realColor = EvalColor( color );

		stream::Tag( colorLiteral, FormatRgbColor( realColor ), pSep );
		stream::Tag( colorLiteral, FormatHtmlColor( realColor ), pSep );
		stream::Tag( colorLiteral, FormatHexColor( realColor ), pSep );
		return colorLiteral;
	}

	bool ParseUndefinedColor( COLORREF* pOutColor, const TCHAR* pColorLiteral )
	{
		ASSERT_PTR( pColorLiteral );

		if ( str::FindStr<str::IgnoreCase>( pColorLiteral, color::g_nullTag ) != utl::npos )
			utl::AssignPtr( pOutColor, CLR_NONE );
		else if ( str::FindStr<str::IgnoreCase>( pColorLiteral, color::g_autoTag ) != utl::npos )
			utl::AssignPtr( pOutColor, CLR_DEFAULT );
		else
			return false;			// not a Windows-undefined CLR_* color

		return true;
	}

	bool ParseColor( OUT COLORREF* pOutColor, const TCHAR* pColorLiteral )
	{
		if ( ParseUndefinedColor( pOutColor, pColorLiteral ) )
			return true;

		if ( ParseSystemColor( pOutColor, pColorLiteral ) )
			return true;

		if ( ParseRgbColor( pOutColor, pColorLiteral ) )
			return true;

		if ( ParseHtmlColor( pOutColor, pColorLiteral ) )
			return true;

		if ( ParseHexColor( pOutColor, pColorLiteral ) )
			return true;

		return false;
	}


	std::tstring FormatSysColor( COLORREF color, bool inColorEntry /*= false*/ )
	{
		ASSERT( IsSysColor( color ) );

		std::tostringstream oss;

		oss << color::g_sysTag << '(' << GetSysColorIndex( color ) << ')';

		if ( !inColorEntry )		// avoid double tagging the system color when is in a color table entry
			if ( const CColorEntry* pSysColorName = CColorRepository::Instance()->GetSystemColorTable()->FindColor( color ) )
				oss << "  \"" << pSysColorName->GetName() << "\"";

		return oss.str();
	}

	std::tstring FormatRgbColor( COLORREF color )
	{
		ASSERT( IsRealColor( color ) );

		return str::Format( _T("%s(%u, %u, %u)"), color::g_rgbTag.c_str(), GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}

	std::tstring FormatHtmlColor( COLORREF color )
	{
		ASSERT( IsRealColor( color ) );

		return str::Format( _T("%s%02X%02X%02X"), color::g_htmlTag.c_str(), GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}

	std::tstring FormatHexColor( COLORREF color )
	{
		ASSERT( IsRealColor( color ) );

		return str::Format( _T("0x%06X"), color );
	}


	bool ParseSystemColor( OUT COLORREF* pOutSysColor, const TCHAR* pSysColorText )
	{	// format: 'SYS(16)  "Button Shadow"'
		ASSERT_PTR( pSysColorText );

		size_t pos = str::FindStr<str::Case>( pSysColorText, color::g_sysTag );

		if ( pos != utl::npos )
		{
			const TCHAR* pText = str::SkipWhitespace( pSysColorText + pos + color::g_sysTag.length() );
			TSysColorIndex sysColorIndex = -1;

			if ( 1 == _stscanf( pText, _T("(%d)"), &sysColorIndex ) )
				if ( ui::IsValidSysColorIndex( sysColorIndex ) )
				{
					utl::AssignPtr( pOutSysColor, MakeSysColor( sysColorIndex ) );
					return true;
				}
		}

		//TRACE( _T("Invalid System Colour literal '%s'\n"), pSysColorText );
		return false;
	}

	bool ParseRgbColor( OUT COLORREF* pOutColor, const TCHAR* pRgbColorText )
	{
		size_t pos = str::FindStr<str::Case>( pRgbColorText, color::g_rgbTag );

		if ( pos != utl::npos )
		{	// RGB( 255, 0, 0 ) for bright red
			const TCHAR* pText = str::SkipWhitespace( pRgbColorText + pos + color::g_rgbTag.length() );
			unsigned int red, green, blue;

			if ( 3 == _stscanf( pText, _T( "(%u,%u,%u)" ), &red, &green, &blue ) )
				if ( red < 256 && green < 256 && blue < 256 )
				{
					utl::AssignPtr( pOutColor, RGB( red, green, blue ) );
					return true;
				}
		}

		//TRACE( _T("Invalid RGB colour literal '%s'\n"), pRgbColorText );
		return false;
	}

	bool ParseHtmlColor( OUT COLORREF* pOutColor, const TCHAR* pHtmlColorText )
	{
		// quick and less safe parsing
		// HTML color format: #RRGGBB - "#FF0000" for bright red
		const TCHAR* pText = str::SkipWhitespace( pHtmlColorText );

		if ( '#' == pText[ 0 ] && str::GetLength( pText ) >= 7 )
		{
			const TCHAR digits[] =
			{
				*++pText, *++pText, '-',
				*++pText, *++pText, '-',
				*++pText, *++pText, '\0'
			};

			unsigned int red, green, blue;
			if ( 3 == _stscanf( digits, _T( "%x-%x-%x" ), &red, &green, &blue ) )
				if ( red < 256 && green < 256 && blue < 256 )
				{
					utl::AssignPtr( pOutColor, RGB( red, green, blue ) );
					return true;
				}
		}

		//TRACE( _T("Invalid HTML colour literal '%s'\n"), pHtmlColorText );
		return false;
	}

	bool ParseHexColor( OUT COLORREF* pOutColor, const TCHAR* pHexColorText )
	{
		size_t pos = str::FindStr<str::IgnoreCase>( pHexColorText, color::g_hexTag );
		if ( pos != utl::npos )
		{
			COLORREF rgbColor;

			if ( 1 == _stscanf( pHexColorText, _T( "0x%06X" ), &rgbColor ) )
				if ( IsRealColor( rgbColor ) )
				{
					utl::AssignPtr( pOutColor, rgbColor );
					return true;
				}
		}

		//TRACE( _T("Invalid hex colour literal '%s'\n"), pHexColorText );
		return false;
	}


	bool CopyColor( COLORREF color )
	{
		bool succeeded = false;
		std::auto_ptr<CTextClipboard> pClipboard( CTextClipboard::Open( AfxGetMainWnd()->GetSafeHwnd() ) );

		if ( pClipboard.get() != nullptr )
		{
			pClipboard->Clear();

			if ( pClipboard->WriteString( FormatColor( color ) ) )
				succeeded = true;

			if ( pClipboard->Write( ui::GetColorClipboardFormat(), color ) )
				succeeded = true;
		}

		return succeeded;
	}

	bool PasteColor( OUT COLORREF* pOutColor )
	{
		std::auto_ptr<CTextClipboard> pClipboard( CTextClipboard::Open( AfxGetMainWnd()->GetSafeHwnd() ) );

		if ( pClipboard.get() != nullptr )
		{
			COLORREF color;
			if ( pClipboard->Read( GetColorClipboardFormat(), color ) )
			{
				utl::AssignPtr( pOutColor, color );
				return true;
			}

			std::tstring text;
			if ( pClipboard->ReadString( text ) )
				if ( ParseColor( pOutColor, text.c_str() ) )
					return true;
		}

		return false;
	}

	bool CanPasteColor( void )
	{
		COLORREF color;
		return PasteColor( &color );
	}

	UINT GetColorClipboardFormat( void )
	{
		static const UINT s_colorFormat = RegisterClipboardFormat( _T("UTL Color") );
		return s_colorFormat;
	}
}


namespace ui
{
	inline double Square( UINT comp ) { ASSERT( comp >= 0 && comp <= 255 ); return static_cast<double>( comp * comp ); }
	inline TFactor PercentToFactor( TPercent pct ) { ASSERT( pct >= 0 && pct <= 100 ); return static_cast<TFactor>( pct ) / 100.0; }
	inline BYTE AsComponent( double component ) { ASSERT( component >= 0.0 && component < 256.0 ); return static_cast<BYTE>( component ); }


	// accurate colour algorithms

	BYTE GetAverageComponent( UINT comp1, UINT comp2 )		// cast to UINT to allow squaring
	{
		// Average by extracting square root of sum of square components (same for G, B): NewColorR = sqrt( (R1^2 + R2^2) / 2 )
		// Details: check arntjw's entry at:
		//	https://stackoverflow.com/questions/649454/what-is-the-best-way-to-average-two-colors-that-define-a-linear-gradient?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

		return AsComponent( sqrt( ( Square(comp1) + Square(comp2) ) / 2 ) );
	}

	BYTE GetAverageComponent( UINT comp1, UINT comp2, UINT comp3 )
	{
		// Average of 3 colours (same for G, B): NewColorR = sqrt( (R1^2 + R2^2 + R3^2) / 3 )

		return AsComponent( sqrt( ( Square(comp1) + Square(comp2) + Square(comp3) ) / 3 ) );
	}

	BYTE GetWeightedMixComponent( UINT comp1, UINT comp2, TFactor weight1 )
	{
		// For weighted mixes (i.e. 75% colour A, and 25% colour B) (same for G, B): NewColorR = sqrt( R1^2 * w + R2^2 * (1 - w) )
		//   weight1 from 0.0 to 1.0

		ASSERT( weight1 >= 0.0 && weight1 <= 1.0 );
		return AsComponent( sqrt( ( Square(comp1) * ( 1.0 - weight1 ) + Square(comp2) * weight1 ) ) );
	}


	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2 )
	{
		if ( CLR_NONE == color1 )
			return color2;
		else if ( CLR_NONE == color2 )
			return color1;

		ASSERT( IsRealColor( color1 ) && IsRealColor( color2 ) );

		return RGB(
			GetAverageComponent( GetRValue( color1 ), GetRValue( color2 ) ),
			GetAverageComponent( GetGValue( color1 ), GetGValue( color2 ) ),
			GetAverageComponent( GetBValue( color1 ), GetBValue( color2 ) )
		);
	}

	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2, COLORREF color3 )
	{
		ASSERT( IsRealColor( color1 ) && IsRealColor( color2 ) && IsRealColor( color3 ) );

		return RGB(
			GetAverageComponent( GetRValue( color1 ), GetRValue( color2 ), GetRValue( color3 ) ),
			GetAverageComponent( GetGValue( color1 ), GetGValue( color2 ), GetGValue( color3 ) ),
			GetAverageComponent( GetBValue( color1 ), GetBValue( color2 ), GetBValue( color3 ) )
		);
	}

	COLORREF GetWeightedMixColor( COLORREF color1, COLORREF color2, TPercent pct1 )
	{
		ASSERT( IsRealColor( color1 ) && IsRealColor( color2 ) );

		TFactor weight1 = PercentToFactor( pct1 );

		return RGB(
			GetWeightedMixComponent( GetRValue( color1 ), GetRValue( color2 ), weight1 ),
			GetWeightedMixComponent( GetGValue( color1 ), GetGValue( color2 ), weight1 ),
			GetWeightedMixComponent( GetBValue( color1 ), GetBValue( color2 ), weight1 )
		);
	}


	COLORREF GetAdjustLuminance( COLORREF color, TPercent byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleLuminance( byPct ).GetRGB();
	}

	COLORREF GetAdjustSaturation( COLORREF color, TPercent byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleSaturation( byPct ).GetRGB();
	}

	COLORREF GetAdjustHue( COLORREF color, TPercent byPct )
	{
		CHslColor hslColor( color );
		return hslColor.ScaleHue( byPct ).GetRGB();
	}


	// CHslColor implementation

	const Range<WORD> CHslColor::s_validRange( 0, 240 );

	CHslColor::CHslColor( COLORREF rgbColor )
	{
		if ( !ui::IsRealColor( rgbColor ) )
			rgbColor = ui::EvalColor( rgbColor );

		::ColorRGBToHLS( rgbColor, &m_hue, &m_luminance, &m_saturation );	// note the difference: Hsl vs HLS - L and S position is swapped in shell API
	}

	COLORREF CHslColor::GetRGB( void ) const
	{
		ASSERT( IsValid() );
		return ::ColorHLSToRGB( m_hue, m_luminance, m_saturation );			// note the difference: Hsl vs HLS - L and S position is swapped in shell API
	}

	WORD CHslColor::ModifyBy( WORD component, TPercent byPercentage )
	{
		ASSERT( byPercentage >= -100 && byPercentage <= 100 );

		const TFactor byFactor = static_cast<TFactor>( byPercentage ) / 100.0;
		double newComponent = component;

		if ( byFactor > 0.0 )
			newComponent += ( ( 255.0 - newComponent ) * byFactor );
		else if ( byFactor < 0.0 )
			newComponent *= ( 1.0 + byFactor );

		static const Range<double> s_dValidRange( s_validRange );
		s_dValidRange.Constrain( newComponent );

		return static_cast<WORD>( newComponent );
	}
}


namespace ui
{
	void MakeHalftoneColorTable( OUT std::vector<COLORREF>& rColorTable, size_t size )
	{
		std::vector<RGBQUAD> rgbTable;
		halftone::MakeRgbTable( rgbTable, size );

		rColorTable.resize( rgbTable.size() );
		std::transform( rgbTable.begin(), rgbTable.end(), rColorTable.begin(), func::ToColor() );
	}


	namespace halftone
	{
		void QueryRgbTableHalftone256( OUT std::vector<RGBQUAD>& rRgbTable )
		{
			std::auto_ptr<CPalette> pPalette( new CPalette() );
			{
				CWindowDC screenDC( nullptr );
				pPalette->CreateHalftonePalette( &screenDC );
			}

			std::vector<PALETTEENTRY> entries;
			entries.resize( pPalette->GetEntryCount() );
			pPalette->GetPaletteEntries( 0, (UINT)entries.size(), &entries.front() );

			rRgbTable.reserve( entries.size() );
			for ( std::vector<PALETTEENTRY>::const_iterator itEntry = entries.begin(); itEntry != entries.end(); ++itEntry )
				rRgbTable.push_back( gdi::ToRgbQuad( *itEntry ) );
		}

		void MakeRgbTable( OUT RGBQUAD* pRgbTable, size_t size )
		{
			ASSERT( size != 0 && 0 == ( size % 2 ) );		// size must be multiple of 2
			ASSERT( size <= 256 );

			std::vector<RGBQUAD> sysTable;
			QueryRgbTableHalftone256( sysTable );

			const size_t halfSize = size >> 1;

			// copy system colors in 2 chunks: first half, last half
			utl::Copy( sysTable.begin(), sysTable.begin() + halfSize, pRgbTable );				// first half from the beginning
			utl::Copy( sysTable.end() - halfSize, sysTable.end(), pRgbTable + halfSize );		// second half from the end
		}
	}
}
