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


namespace d2d
{
	interface IDeviceComponent
	{	// manages device-dependent resources (bitmaps, brushes, text layouts, etc)
		virtual void DiscardDeviceResources( void ) = 0;
		virtual bool CreateDeviceResources( void ) = 0;
	};


	interface IRenderHost;
	struct CViewCoords;


	interface IGadgetComponent : public IDeviceComponent
	{	// a child gadget managed by a render host that can draw itself
		virtual IRenderHost* GetRenderHost( void ) const = 0;
		virtual bool IsValid( void ) const = 0;

		// draw stages (in relation with drawing the content)
		virtual void EraseBackground( const CViewCoords& coords ) = 0;
		virtual void Draw( const CViewCoords& coords ) = 0;
	};



	interface IRenderHost
	{	// provides access to the render target managed by the host, a composite of gadgets
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const = 0;
		virtual bool CanRender( void ) const = 0;

		// composite of gadgets
		virtual void AddGadget( IGadgetComponent* pGadget ) = 0;
		virtual bool IsGadgetVisible( const IGadgetComponent* pGadget ) const = 0;		// allows the host to prevent drawing of a certain gadget

		bool IsValidTarget( void ) const { return GetRenderTarget() != NULL; }
	};



	interface IRenderHostWindow : public utl::IMemoryManaged
								, public IRenderHost
	{	// render host window capable of animation
		virtual CWnd* GetWindow( void ) const = 0;

		virtual void StartAnimation( UINT frameDelay ) = 0;
		virtual void StopAnimation( void ) = 0;
	};
}


namespace d2d
{
	struct CViewCoords : private utl::noncopyable
	{
		CViewCoords( const CRect& clientRect, const CRect& contentRect )
			: m_clientRect( clientRect )
			, m_contentRect( contentRect )
		{
		}
	public:
		const CRect& m_clientRect;
		const CRect& m_contentRect;			// logical coordinates: bitmap scaled rect in the view (AKA destRect)
	};


	abstract class CGadgetBase : public IGadgetComponent
							   , private utl::noncopyable
	{
	protected:
		CGadgetBase( void ) : m_pRenderHost( NULL ) {}
	public:
		// IGadgetComponent interface (partial)
		virtual IRenderHost* GetRenderHost( void ) const;
		virtual void EraseBackground( const CViewCoords& coords );

		void SetRenderHost( IRenderHost* pRenderHost );
	protected:
		virtual ID2D1RenderTarget* GetHostRenderTarget( void ) const { return GetRenderHost()->GetRenderTarget(); }
	private:
		IRenderHost* m_pRenderHost;
	};
}


#endif // Direct2D_h
