#ifndef Color_h
#define Color_h
#pragma once

#include "StdColors.h"
#include "Range.h"


namespace ui
{
	typedef int TSysColorIndex;		// [COLOR_SCROLLBAR, COLOR_MENUBAR]
	typedef int TPercent;			// [0, 100] or [-100, 100]
	typedef double TFactor;			// [0.0, 1.0] or [-1.0, 1.0]


	inline bool IsNullColor( COLORREF rawColor ) { return CLR_NONE == rawColor; }
	inline bool IsDefaultColor( COLORREF rawColor ) { return CLR_DEFAULT == rawColor; }

	inline bool IsUndefinedColor( COLORREF rawColor ) { return CLR_NONE == rawColor || CLR_DEFAULT == rawColor; }


	enum StdTranspColor { Transp_LowColor = color::LightGrey, Transp_TrueColor = color::ToolStripPink };

	inline COLORREF GetStdTranspColor( WORD bitsPerPixel ) { return bitsPerPixel >= 24 ? Transp_TrueColor : Transp_LowColor; }


	namespace color_info
	{
		enum Mask { MaskRgb = (COLORREF)0x00FFFFFF, MaskSysIndex = (COLORREF)0x000000FF, MaskColorFlags = (COLORREF)0xFF000000 };
		enum Flag { FlagSysColorIndex = (COLORREF)0x80000000, FlagPaletteIndex = (COLORREF)0x40000000 };
	}


	inline BYTE GetColorFlags( COLORREF color ) { return LOBYTE( color >> 24 ); }				// highest BYTE


	// system color index
	inline bool IsSysColor( COLORREF color ) { return !IsUndefinedColor( color ) && color_info::FlagSysColorIndex == ( color & color_info::MaskColorFlags ); }
		inline bool IsValidSysColorIndex( TSysColorIndex sysColorIndex ) { return ::GetSysColorBrush( sysColorIndex ) != nullptr; }
		inline TSysColorIndex GetSysColorIndex( COLORREF color ) { ASSERT( IsSysColor( color ) ); return color & color_info::MaskSysIndex; }
	inline COLORREF MakeSysColor( ui::TSysColorIndex sysColorIndex ) { ASSERT( IsValidSysColorIndex( sysColorIndex ) ); return color_info::FlagSysColorIndex | sysColorIndex; }

	COLORREF EvalColor( COLORREF rawColor );

	inline bool IsActualColor( COLORREF rawColor ) { return 0 == GetColorFlags( rawColor ); }			// not CLR_NONE, CLR_DEFAULT, other
	inline COLORREF GetActualColor( COLORREF rawColor, COLORREF defaultColor ) { return IsActualColor( rawColor ) ? rawColor : defaultColor; }
	inline COLORREF GetActualColorSysdef( COLORREF rawColor, TSysColorIndex defaultSysIndex ) { return IsActualColor( rawColor ) ? rawColor : ::GetSysColor( defaultSysIndex ); }
}


#define HTML_COLOR( htmlLiteral ) ( ui::ParseHtmlColor( #htmlLiteral ) )


namespace ui
{
	// string conversions
	std::tstring FormatColor( COLORREF color, const TCHAR* pSep = _T("  ") );
	bool ParseColor( OUT COLORREF* pOutColor, const TCHAR* pColorLiteral );

	std::tstring FormatRgbColor( COLORREF color );
	std::tstring FormatHtmlColor( COLORREF color );
	std::tstring FormatSysColor( COLORREF color );

	bool ParseRgbColor( OUT COLORREF* pOutColor, const TCHAR* pRgbColorText );
	bool ParseHtmlColor( OUT COLORREF* pOutColor, const TCHAR* pHtmlColorText );
	bool ParseSystemColor( OUT COLORREF* pOutSysColor, const TCHAR* pSysColorText );


	// clipboard
	bool CopyColor( COLORREF color );
	bool PasteColor( OUT COLORREF* pOutColor );
	static bool CanPasteColor( void );
	static UINT GetColorClipboardFormat( void );

	UINT GetColorClipboardFormat( void );
}


namespace ui
{
	BYTE GetLuminance( UINT red, UINT green, UINT blue );

	inline BYTE GetLuminance( COLORREF color )
	{
		return GetLuminance( GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}

	inline COLORREF GetContrastColor( COLORREF bkColor, COLORREF darkColor = color::Black, COLORREF lightColor = color::White )
	{
		return GetLuminance( bkColor ) < 128 ? lightColor : darkColor;
	}

	inline COLORREF GetMappedColor( COLORREF color, CDC* pDC )
	{
		// CDC::GetNearestColor() doesn't produce the expected result for a DIB selected in it.
		// we need to call SetPixel to get the right colour mapping, which is equivalent to BitBlt() colour mapping.
		COLORREF originColor = pDC->GetPixel( 0, 0 );			// save pixel at origin
		COLORREF mappedColor = pDC->SetPixel( 0, 0, color );
		pDC->SetPixel( 0, 0, originColor );						// restore pixel at origin
		return mappedColor;
	}
}


namespace ui
{
	// accurate colour algorithms (using geometric averaging)

	BYTE GetAverageComponent( UINT comp1, UINT comp2 );
	BYTE GetAverageComponent( UINT comp1, UINT comp2, UINT comp3 );
	BYTE GetWeightedMixComponent( UINT comp1, UINT comp2, TFactor weight1 );		// weight1 from 0.0 to 1.0

	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2 );
	COLORREF GetBlendedColor( COLORREF color1, COLORREF color2, COLORREF color3 );
	COLORREF GetWeightedMixColor( COLORREF color1, COLORREF color2, TPercent pct1 );

	inline COLORREF& BlendWithColor( COLORREF& rColor1, COLORREF color2 ) { return rColor1 = GetBlendedColor( rColor1, color2 ); }
	inline COLORREF& WeightedMixWithColor( COLORREF& rColor1, COLORREF color2, TPercent pct1 ) { return rColor1 = GetWeightedMixColor( rColor1, color2, pct1 ); }


	// HSL adjustments

	COLORREF GetAdjustLuminance( COLORREF color, TPercent byPct );
	COLORREF GetAdjustSaturation( COLORREF color, TPercent byPct );
	COLORREF GetAdjustHue( COLORREF color, TPercent byPct );

	inline COLORREF& AdjustLuminance( COLORREF& rColor, TPercent byPct ) { return rColor = GetAdjustLuminance( rColor, byPct ); }
	inline COLORREF& AdjustSaturation( COLORREF& rColor, TPercent byPct ) { return rColor = GetAdjustSaturation( rColor, byPct ); }
	inline COLORREF& AdjustHue( COLORREF& rColor, TPercent byPct ) { return rColor = GetAdjustHue( rColor, byPct ); }


	struct CHslColor
	{
	public:
		CHslColor( void ) : m_hue( 0 ), m_saturation( 100 ), m_luminance( 0 ) {}
		CHslColor( WORD hue, WORD saturation, WORD luminance ) : m_hue( hue ), m_saturation( saturation ), m_luminance( luminance ) {}
		CHslColor( COLORREF rgbColor );

		bool IsValid( void ) const { return s_validRange.Contains( m_hue ) && s_validRange.Contains( m_saturation ) && s_validRange.Contains( m_luminance ); }
		COLORREF GetRGB( void ) const;

		CHslColor& ScaleHue( TPercent byPct ) { m_hue = ModifyBy( m_hue, byPct ); return *this; }
		CHslColor& ScaleSaturation( TPercent byPct ) { m_saturation = ModifyBy( m_saturation, byPct ); return *this; }
		CHslColor& ScaleLuminance( TPercent byPct ) { m_luminance = ModifyBy( m_luminance, byPct ); return *this; }

		CHslColor GetScaledHue( TPercent byPct ) { CHslColor newColor = *this; return newColor.ScaleHue( byPct ); }
		CHslColor GetScaledSaturation( TPercent byPct ) { CHslColor newColor = *this; return newColor.ScaleSaturation( byPct ); }
		CHslColor GetScaledLuminance( TPercent byPct ) { CHslColor newColor = *this; return newColor.ScaleLuminance( byPct ); }

		static WORD ModifyBy( WORD component, TPercent byPercentage );
	public:
		WORD m_hue;
		WORD m_saturation;
		WORD m_luminance;

		static const Range<WORD> s_validRange;		// [0, 240]
	};
}


namespace ui
{
	struct CColorAlpha
	{
		CColorAlpha( void ) : m_color( CLR_NONE ), m_alpha( 0xFF ) {}
		CColorAlpha( COLORREF color, BYTE alpha ) : m_color( color ), m_alpha( alpha ) {}
		CColorAlpha( BYTE red, BYTE green, BYTE blue, BYTE alpha ) : m_color( RGB( red, green, blue ) ), m_alpha( alpha ) {}

		bool IsNull( void ) const { return CLR_NONE == m_color || IsDefault(); }
		bool IsDefault( void ) const { return CLR_DEFAULT == m_color; }

		static CColorAlpha MakeSysColor( TSysColorIndex sysColorIndex, BYTE alpha = 0xFF ) { return CColorAlpha( ::GetSysColor( sysColorIndex ), alpha ); }
		static CColorAlpha MakeTransparent( COLORREF color, UINT transpPct ) { return CColorAlpha( color, MakeAlpha( transpPct ) ); }
		static CColorAlpha MakeOpaqueColor( COLORREF color, UINT opacityPct ) { return CColorAlpha( color, MakeAlpha( 100 - opacityPct ) ); }

		static BYTE FromPercentage( UINT percentage ) { ASSERT( percentage <= 100 ); return static_cast<BYTE>( (TFactor)percentage * 255 / 100 ); }
		static BYTE MakeAlpha( UINT transpPct ) { return FromPercentage( 100 - transpPct ); }
	public:
		COLORREF m_color;
		BYTE m_alpha;
	};
}


#endif // Color_h
