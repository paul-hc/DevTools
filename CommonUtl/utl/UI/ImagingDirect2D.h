#ifndef ImagingDirect2D_h
#define ImagingDirect2D_h
#pragma once

#include <d2d1.h>
#include "ImagingWic_fwd.h"
#include "InternalChange.h"
#include "ISubject.h"


// D2D: Direct 2D


namespace d2d
{
	// type conversions: GDI -> D2D

	inline D2D_POINT_2U ToPoint( const CPoint& pos ) { return D2D1::Point2U( pos.x, pos.y ); }
	inline D2D_POINT_2F ToPointF( const CPoint& pos ) { return D2D1::Point2F( (float)pos.x, (float)pos.y ); }

	inline D2D_SIZE_U ToSize( const CSize& size ) { return D2D1::SizeU( size.cx, size.cy ); }
	inline D2D_SIZE_F ToSizeF( const CSize& size ) { return D2D1::SizeF( (float)size.cx, (float)size.cy ); }

	inline D2D_RECT_U ToRect( const CRect& rect ) { return D2D1::RectU( rect.left, rect.top, rect.right, rect.bottom ); }
	inline D2D_RECT_F ToRectF( const CRect& rect ) { return D2D1::RectF( (float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom ); }

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

	inline float GetAlpha( WICColor argbColor ) { return ( argbColor >> 24 ) / 255.f; }


	// type conversions: D2D -> GDI
	inline CSize FromSize( const D2D_SIZE_U& size ) { return CSize( size.width, size.height ); }
}


namespace d2d
{
	// singleton to create WIC COM objects for
	//
	class CImagingFactory
	{
		CImagingFactory( void );
		~CImagingFactory();
	public:
		static CImagingFactory& Instance( void );
		static ID2D1Factory* Factory( void ) { return &*Instance().m_pFactory; }
	private:
		CComPtr< ID2D1Factory > m_pFactory;
	};


	enum RenderResult { RenderDone, RenderError, DeviceLoss };


	// renders a D2D bitmap to any type of render target (window, DC, etc)
	//
	struct CDrawBitmapTraits
	{
		CDrawBitmapTraits( COLORREF bkColor = CLR_NONE, bool smoothing = true, UINT opacityPct = 100 )
			: m_bkColor( bkColor )
			, m_opacity( static_cast< float >( opacityPct ) / 100.f )
			, m_interpolationMode( smoothing ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR )
			, m_transform( D2D1::Matrix3x2F::Identity() )
			, m_frameColor( CLR_NONE )
		{
		}

		void SetAutoInterpolationMode( const CSize& destBoundsSize, const CSize& bmpSize );

		void Draw( ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap* pBitmap, const CRect& destRect, const CRect* pSrcRect = NULL ) const;

		static bool IsSmoothingMode( void ) { return s_enlargeInterpolationMode != D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR; }
		static bool SetSmoothingMode( bool smoothingMode = true );
	public:
		COLORREF m_bkColor;
		float m_opacity;
		D2D1_BITMAP_INTERPOLATION_MODE m_interpolationMode;
			// D2D1_BITMAP_INTERPOLATION_MODE_LINEAR: pixel smoothing (default)
			// D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR: no dithering (accurate pixel scaling)
		D2D1_MATRIX_3X2_F m_transform;			// Identity, Scale, Translation, Rotation, etc
		COLORREF m_frameColor;					// useful for debugging

		static D2D1_BITMAP_INTERPOLATION_MODE s_enlargeInterpolationMode;		// by default no smoothing when enlarging images (raster image friendly)
	};


	interface IDeviceResources : public IMemoryManaged
	{
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const = 0;
		virtual CWnd* GetWindow( void ) const = 0;
		virtual void DiscardResources( void ) = 0;
		virtual bool CreateResources( void ) = 0;
		virtual bool CanDraw( void ) const { return IsValid(); }

		bool IsValid( void ) const { return GetRenderTarget() != NULL; }

		void EnsureResources( void )
		{
			if ( !IsValid() )
				CreateResources();			// lazy resource aquisition
		}
	};


	abstract class CRenderTarget : public IDeviceResources
								 , protected CInternalChange
								 , private utl::noncopyable
	{
	protected:
		CRenderTarget( void ) {}

		// base overrides
		virtual void OnFirstAddInternalChange( void );				// calls BeginDraw()
		virtual void OnFinalReleaseInternalChange( void );			// calls EndDraw()
	public:
		IWICBitmapSource* GetWicBitmap( void ) const { return m_pWicBitmap; }
		void SetWicBitmap( IWICBitmapSource* pWicBitmap );

		void ClearBackground( COLORREF bkColor );						// clears the entire render target

		ID2D1Bitmap* GetBitmap( void );
		RenderResult DrawBitmap( const CDrawBitmapTraits& traits, const CRect& destRect, const CRect* pSrcRect = NULL );	// invalidates window on device loss
	protected:
		void ReleaseBitmap( void ) { m_pBitmap = NULL; }
	private:
		CComPtr< IWICBitmapSource > m_pWicBitmap;						// source bitmap
		CComPtr< ID2D1Bitmap > m_pBitmap;								// self-encapsulated, released on device loss

		// optional drawing resources
		CComPtr< ID2D1SolidColorBrush > m_pAccentBrush;
	};


	typedef CScopedInternalChange CScopedDraw;


	// D2D window render target paired with a bitmap (base class) that get recreated after a device loss
	//
	class CWindowRenderTarget : public CRenderTarget
	{
	public:
		CWindowRenderTarget( CWnd* pWnd ) : CRenderTarget(), m_pWnd( pWnd ) { ASSERT_PTR( m_pWnd ); }

		bool IsValid( void ) const { return m_pWndRenderTarget != NULL; }

		bool Resize( const CSize& clientSize );
		bool Resize( void );

		// IDeviceResources interface
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const { return m_pWndRenderTarget; }
		virtual CWnd* GetWindow( void ) const { return m_pWnd; }
		virtual void DiscardResources( void );
		virtual bool CreateResources( void );
		virtual bool CanDraw( void ) const;
	private:
		CWnd* m_pWnd;
		CComPtr< ID2D1HwndRenderTarget > m_pWndRenderTarget;		// self-encapsulated, released on device loss
	};


	// D2D DC render target paired with a bitmap (base class)
	//
	class CDCRenderTarget : public CRenderTarget
	{
	public:
		CDCRenderTarget( CDC* pDC )
			: CRenderTarget()
			, m_pDC( pDC )
		{
			ASSERT_PTR( m_pDC->GetSafeHdc() );
			ASSERT_PTR( m_pDC->GetWindow() );						// DC's window is required for getting the client rect
		}

		bool Resize( const CRect& subRect );

		// IDeviceResources interface
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const { return m_pDcRenderTarget; }
		virtual CWnd* GetWindow( void ) const { return safe_ptr( m_pDC->GetWindow() ); }
		virtual void DiscardResources( void );
		virtual bool CreateResources( void );
	private:
		CDC* m_pDC;
		CComPtr< ID2D1DCRenderTarget > m_pDcRenderTarget;			// self-encapsulated, released on device loss
	};
}


#endif // ImagingDirect2D_h
