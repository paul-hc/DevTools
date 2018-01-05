#ifndef Color_h
#define Color_h
#pragma once

#include "StdColors.h"


#define HTML_COLOR( htmlLiteral ) ( ui::ParseHtmlLiteral( #htmlLiteral ) )


namespace ui
{
	enum StdTranspColor { Transp_LowColor = color::LightGrey, Transp_TrueColor = color::ToolStripPink };

	inline COLORREF GetStdTranspColor( WORD bitsPerPixel ) { return bitsPerPixel >= 24 ? Transp_TrueColor : Transp_LowColor; }

	COLORREF ParseHtmlLiteral( const char* pHtmlLiteral );		// quick and less safe parsing

	std::tstring FormatRgb( COLORREF color );
	std::tstring FormatHtml( COLORREF color );

	bool ParseRgb( COLORREF* pColor, const std::tstring& text );
	bool ParseHtml( COLORREF* pColor, const std::tstring& text );
	bool ParseSys( COLORREF* pColor, const std::tstring& text );

	std::tstring FormatColor( COLORREF color, const TCHAR* pSep = _T("  ") );
	bool ParseColor( COLORREF* pColor, const std::tstring& text );


	inline BYTE GetLuminance( BYTE red, BYTE green, BYTE blue )
	{	// luminance (gray level) of a color (Y component) = 0.3*R + 0.59*G + 0.11*B
		if ( red == green && red == blue )
			return red;							// preserve grays unchanged

		UINT luminance = ( (UINT)red * 30 + green * 59 + blue * 11 ) / 100;
		return static_cast< BYTE >( luminance );
	}

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


#endif // Color_h
