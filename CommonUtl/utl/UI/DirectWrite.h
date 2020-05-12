#ifndef DirectWrite_h
#define DirectWrite_h
#pragma once

#include <dwrite.h>
#include "Direct2D.h"


namespace dw
{
	// singleton to create Direct Write COM objects for drawing text with:
	//	- Direct2D (hardware acceleration)
	//	- GDI or GDI+
	//
	class CTextFactory
	{
		CTextFactory( void );
		~CTextFactory();
	public:
		static ::IDWriteFactory* Factory( void );
	private:
		CComPtr< ::IDWriteFactory > m_pDirectWriteFactory;
	};
}


namespace dw
{
	float ConvertPointToDipExtent( int pointExtent, float dpiHeight = d2d::GetScreenDpi().height );
	int ConvertDipToPointExtent( float dipExtent, float dpiHeight = d2d::GetScreenDpi().height );

	D2D_SIZE_F ConvertSizeToDip( const CSize& size, const D2D_SIZE_F& dpiSize = d2d::GetScreenDpi() );
	CSize ConvertDipToSize( const D2D_SIZE_F& dipSize, const D2D_SIZE_F& dpiSize = d2d::GetScreenDpi() );


	// text format is similar to font in GDI, used for rendering text (device-independent)
	CComPtr< IDWriteTextFormat > CreateTextFormat( const wchar_t fontName[], long height,
												   DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR,
												   DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
												   DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL,
												   const wchar_t localeName[] = L"en-us" );

	D2D_SIZE_F GetBoundsSize( IDWriteTextLayout* pTextLayout );
	D2D_SIZE_F ComputeTextSize( IDWriteTextLayout* pTextLayout );
	D2D_SIZE_F ComputeOverhangTextSize( IDWriteTextLayout* pTextLayout );

	D2D_SIZE_F ComputeTextSize( IDWriteTextFormat* pFont, const wchar_t text[], size_t length = utl::npos );
}


namespace dw
{
	// stores the full text of the text-layout, structured in fields with effect ranges
	class CTextLayout
	{
	public:
		CTextLayout( size_t fieldCount, const TCHAR fieldSep[] )
			: m_fieldCount( fieldCount )
			, m_pFieldSep( fieldSep )
		{
			m_fullText.reserve( 256 );
		}

		void AddField( const std::tstring& fieldText, const TCHAR* pFieldSep = NULL );

		const std::tstring& GetFullText( void ) const { return m_fullText; }
		UINT GetFullTextLength( void ) const { return static_cast<UINT>( m_fullText.length() ); }

		const DWRITE_TEXT_RANGE& GetFieldRangeAt( size_t field ) const { ASSERT( field < m_fieldRanges.size() ); return m_fieldRanges[ field ]; }
	private:
		size_t m_fieldCount;
		const TCHAR* m_pFieldSep;

		std::tstring m_fullText;
		std::vector< DWRITE_TEXT_RANGE > m_fieldRanges;		// indexed by field
	};
}


#endif // DirectWrite_h
