
#include "stdafx.h"
#include "DirectWrite.h"
#include "BaseApp.h"
#include "Utilities.h"

#pragma comment(lib, "dwrite.lib")	// link to DirectWrite

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace dw
{
	// CTextFactory implementation

	CTextFactory::CTextFactory( void )
	{
		HR_OK( ::DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof( m_pDirectWriteFactory ), reinterpret_cast<IUnknown**>( &m_pDirectWriteFactory ) ) );

		app::GetSharedResources().AddComPtr( m_pDirectWriteFactory );			// will release the factory singleton in ExitInstance()
	}

	CTextFactory::~CTextFactory()
	{
	}

	::IDWriteFactory* CTextFactory::Factory( void )
	{
		static CTextFactory s_factory;
		return s_factory.m_pDirectWriteFactory;
	}
}


namespace dw
{
	float ConvertPointToDipExtent( int pointExtent, float dpiHeight /*= d2d::GetScreenDpi().height*/ )
	{
		return static_cast<float>( pointExtent < 0 ? ( -pointExtent ) : pointExtent ) * dpiHeight / 72.0f;
	}

	int ConvertDipToPointExtent( float dipExtent, float dpiHeight /*= d2d::GetScreenDpi().height*/ )
	{
		return d2d::GetCeiling( dipExtent * 72.0f / dpiHeight );
	}


	D2D_SIZE_F ConvertSizeToDip( const CSize& size, const D2D_SIZE_F& dpiSize /*= d2d::GetScreenDpi()*/ )
	{
		return D2D1::SizeF( ConvertPointToDipExtent( size.cx, dpiSize.width ), ConvertPointToDipExtent( size.cy, dpiSize.height ) );
	}

	CSize ConvertDipToSize( const D2D_SIZE_F& dipSize, const D2D_SIZE_F& dpiSize /*= d2d::GetScreenDpi()*/ )
	{
		return CSize( ConvertDipToPointExtent( dipSize.width, dpiSize.width ), ConvertDipToPointExtent( dipSize.height, dpiSize.height ) );
	}


	CComPtr<IDWriteTextFormat> CreateTextFormat( const wchar_t fontName[], long height,
												   DWRITE_FONT_WEIGHT weight /*= DWRITE_FONT_WEIGHT_REGULAR*/,
												   DWRITE_FONT_STYLE style /*= DWRITE_FONT_STYLE_NORMAL*/,
												   DWRITE_FONT_STRETCH stretch /*= DWRITE_FONT_STRETCH_NORMAL*/,
												   const wchar_t localeName[] /*= L"en-us"*/ )
	{
		CComPtr<IDWriteTextFormat> pTextFormat;			// new font
		float fontSize = ConvertPointToDipExtent( height );

		if ( HR_OK( CTextFactory::Factory()->CreateTextFormat( fontName, NULL, weight, style, stretch, fontSize, localeName, &pTextFormat ) ) )
		{
			HR_VERIFY( pTextFormat->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_LEADING ) );
			HR_VERIFY( pTextFormat->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_NEAR ) );
			return pTextFormat;
		}

		return NULL;
	}

    CComPtr<IDWriteTextLayout> CreateTextLayout( const std::tstring& text, IDWriteTextFormat* pFont, D2D_SIZE_F maxSize /*= d2d::GetScreenSize()*/ )
    {
		CComPtr<IDWriteTextLayout> pTextLayout;
		if ( HR_OK( CTextFactory::Factory()->CreateTextLayout( text.c_str(), static_cast<UINT32>( text.length() ), pFont, maxSize.width, maxSize.height, &pTextLayout ) ) )
			return pTextLayout;
		return NULL;
    }


	D2D_SIZE_F GetBoundsSize( IDWriteTextLayout* pTextLayout )
	{
		ASSERT_PTR( pTextLayout );
		return D2D1::SizeF( pTextLayout->GetMaxWidth(), pTextLayout->GetMaxHeight() );
	}

	D2D_SIZE_F ComputeTextSize( IDWriteTextLayout* pTextLayout )
	{
		ASSERT_PTR( pTextLayout );

		D2D_SIZE_F _textMaxSize = GetBoundsSize( pTextLayout ); _textMaxSize;

		D2D_SIZE_F textSize = { 0, 0 };
		DWRITE_TEXT_METRICS textMetrics = { 0 };

		if ( HR_OK( pTextLayout->GetMetrics( &textMetrics ) ) )
			textSize = D2D1::SizeF( textMetrics.widthIncludingTrailingWhitespace, textMetrics.height );
		else
			ASSERT( false );

		return textSize;
	}

	D2D_SIZE_F ComputeOverhangTextSize( IDWriteTextLayout* pTextLayout )
	{
		ASSERT_PTR( pTextLayout );

		D2D_SIZE_F textOverhangSize = { 0, 0 };
		DWRITE_OVERHANG_METRICS overhangMetrics = { 0 };

		if ( HR_OK( pTextLayout->GetOverhangMetrics( &overhangMetrics ) ) )
			textOverhangSize = D2D1::SizeF( -overhangMetrics.left, -overhangMetrics.top );
		else
			ASSERT( false );

		return textOverhangSize;
	}

	D2D_SIZE_F ComputeTextSize( IDWriteTextFormat* pFont, const wchar_t text[], size_t length /*= utl::npos*/ )
	{
		D2D_SIZE_F screenSize = d2d::ToSizeF( ui::GetScreenSize() );

		CComPtr<IDWriteTextLayout> pTextLayout;
		if ( HR_OK( CTextFactory::Factory()->CreateTextLayout( text, static_cast<UINT32>( length ), pFont, screenSize.width, screenSize.height, &pTextLayout ) ) )
			return ComputeTextSize( pTextLayout );

		return D2D1::SizeF();
	}
}


namespace dw
{
	// CTextLayout implementation

	void CTextLayout::AddField( const std::tstring& fieldText, const TCHAR* pFieldSep /*= NULL*/ )
	{
		DWRITE_TEXT_RANGE range = { static_cast<UINT>( m_fullText.length() ), 0 };

		if ( NULL == pFieldSep )
			pFieldSep = m_pFieldSep;

		if ( stream::Tag( m_fullText, fieldText, pFieldSep ) )
			range.length = static_cast<UINT>( m_fullText.length() ) - range.startPosition;

		m_fieldRanges.push_back( range );
	}
}
