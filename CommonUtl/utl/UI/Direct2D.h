#ifndef Direct2D_h
#define Direct2D_h
#pragma once

#include <d2d1.h>
#include "ImagingWic_fwd.h"		// for WICColor


// D2D: Direct 2D


namespace d2d
{
	// singleton to create Direct 2D COM objects
	//
	class CFactory
	{
		CFactory( void );
		~CFactory();
	public:
		static ::ID2D1Factory* Factory( void );
	private:
		CComPtr< ::ID2D1Factory > m_pFactory;
	};
}


namespace d2d
{
	D2D_SIZE_F GetScreenDpi( void );		// usually 96 DPI


	// GDI -> D2D type conversions

	inline D2D_POINT_2U ToPointU( const POINT& pos ) { return D2D1::Point2U( pos.x, pos.y ); }
	inline D2D_POINT_2F ToPointF( const POINT& pos ) { return D2D1::Point2F( (float)pos.x, (float)pos.y ); }
	inline D2D_POINT_2F ToPointF( int x, int y ) { return D2D1::Point2F( (float)x, (float)y ); }

	inline D2D_SIZE_U ToSizeU( const SIZE& size ) { return D2D1::SizeU( size.cx, size.cy ); }
	inline D2D_SIZE_F ToSizeF( const SIZE& size ) { return D2D1::SizeF( (float)size.cx, (float)size.cy ); }
	inline D2D_SIZE_F ToSizeF( int cx, int cy ) { return D2D1::SizeF( (float)cx, (float)cy ); }

	inline D2D_RECT_U ToRectU( const RECT& rect ) { return D2D1::RectU( rect.left, rect.top, rect.right, rect.bottom ); }
	inline D2D_RECT_F ToRectF( const RECT& rect ) { return D2D1::RectF( (float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom ); }


	// D2D -> GDI type conversions

	int GetCeiling( float number );
	int GetRounded( float number );

	inline CSize FromSizeU( const D2D_SIZE_U& size ) { return CSize( size.width, size.height ); }
	inline CSize FromSizeF( const D2D_SIZE_F& size ) { return CSize( GetCeiling( size.width ), GetCeiling( size.height ) ); }
	inline CSize FromSizeF( float width, float height ) { return CSize( GetCeiling( width ), GetCeiling( height ) ); }


	// D2D1_COLOR_F is equivalent with D2D_COLOR_F

	inline D2D1_COLOR_F ToColor( COLORREF color, UINT opacityPct = 100 )
	{
		D2D1_COLOR_F colorValue =		// RGBA
		{
			static_cast< float >( GetRValue( color ) ) / 255.f,
			static_cast< float >( GetGValue( color ) ) / 255.f,
			static_cast< float >( GetBValue( color ) ) / 255.f,
			static_cast< float >( opacityPct ) / 100.f
		};
		return colorValue;
	}

	inline D2D1_COLOR_F ToColorAlpha( COLORREF color, BYTE alpha = 255 )
	{
		D2D1_COLOR_F colorValue =		// RGBA
		{
			static_cast< float >( GetRValue( color ) ) / 255.f,
			static_cast< float >( GetGValue( color ) ) / 255.f,
			static_cast< float >( GetBValue( color ) ) / 255.f,
			static_cast< float >( alpha ) / 255.f
		};
		return colorValue;
	}

	inline float GetAlpha( WICColor argbColor ) { return ( argbColor >> 24 ) / 255.f; }
}


#endif // Direct2D_h
