#ifndef ImagingDirect2D_h
#define ImagingDirect2D_h
#pragma once

#include "Direct2D.h"
#include "InternalChange.h"


namespace d2d
{
	// renders a D2D bitmap to any type of render target (window, DC, etc)
	//
	struct CDrawBitmapTraits
	{
		CDrawBitmapTraits( COLORREF bkColor = CLR_NONE, UINT opacityPct = 100 );

		bool IsSmoothingMode( void ) const { return utl::EvalTernary( m_smoothingMode, CSharedTraits::Instance().IsSmoothingMode() ); }
		utl::Ternary GetSmoothingMode( void ) const { return m_smoothingMode; }
		void SetSmoothingMode( utl::Ternary smoothingMode = utl::True ) { m_smoothingMode = smoothingMode; }

		void SetScrollPos( const POINT& scrollPos );

		void Draw( ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap* pBitmap, const CRect& destRect, const CRect* pSrcRect = nullptr ) const;
	private:
		utl::Ternary m_smoothingMode;			// bitmap drawing strategy for enlarging (scaling up) - could override CSharedTraits::m_pixelSmoothEnlarge
	public:
		COLORREF m_bkColor;
		float m_opacity;
		D2D1::Matrix3x2F m_transform;			// Identity, Scale, Translation, Rotation, etc
		COLORREF m_frameColor;					// useful for debugging
	};


	struct CBitmapCoords : private utl::noncopyable
	{
		CBitmapCoords( const CDrawBitmapTraits& dbmTraits, const CRect* pSrcBmpRect = nullptr )
			: m_dbmTraits( dbmTraits )
			, m_pSrcBmpRect( pSrcBmpRect )
		{
		}
	public:
		const CDrawBitmapTraits& m_dbmTraits;
		const CRect* m_pSrcBmpRect;			// source bitmap area as a subset (default null: entire bitmap)
	};
}


namespace d2d
{
	enum RenderResult { RenderDone, RenderError, DeviceLoss };


	abstract class CRenderTarget : public IRenderHostWindow
		, public IDeviceComponent
		, protected CInternalChange
		, private utl::noncopyable
	{
	protected:
		CRenderTarget( void ) {}
	public:
		// IRenderHost partial interface
		virtual bool CanRender( void ) const;
		virtual void AddGadget( IGadgetComponent* pGadget );
		virtual bool IsGadgetVisible( const IGadgetComponent* pGadget ) const;

		// IRenderHostWindow partial interface (assume no animation)
		virtual void StartAnimation( UINT frameDelay ) { frameDelay; }
		virtual void StopAnimation( void ) {}

		// IDeviceComponent interface (composite of gadgets)
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );
	public:
		IWICBitmapSource* GetWicBitmap( void ) const { return m_pWicBitmap; }
		bool SetWicBitmap( IWICBitmapSource* pWicBitmap );			// returns true if a new bitmap

		ID2D1Bitmap* GetBitmap( void );

		void ClearBackground( COLORREF bkColor );					// clears the entire render target

		RenderResult Render( const CViewCoords& coords, const CBitmapCoords& bmpCoords );			// invalidates window on device loss

		void EnsureDeviceResources( void )
		{
			if ( !IsValidTarget() )
				CreateDeviceResources();			// lazy resource aquisition
		}
	protected:
		void ReleaseBitmap( void ) { m_pBitmap = nullptr; }

		// CInternalChange overrides
		virtual void OnFirstAddInternalChange( void );				// calls BeginDraw()
		virtual void OnFinalReleaseInternalChange( void );			// calls EndDraw()

		// overrideables
		virtual void DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords );
		virtual void PreDraw( const CViewCoords& coords );
		virtual void PostDraw( const CViewCoords& coords );
	private:
		std::vector<IGadgetComponent*> m_gadgets;		// composite of gadgets (no ownership)
		CComPtr<IWICBitmapSource> m_pWicBitmap;		// source bitmap
		CComPtr<ID2D1Bitmap> m_pBitmap;				// self-encapsulated, released on device loss
	};


	typedef CScopedInternalChange TScopedDraw;


	// D2D window render target paired with a bitmap (base class) that get recreated after a device loss
	//
	class CWindowRenderTarget : public CRenderTarget
	{
	public:
		CWindowRenderTarget( CWnd* pWnd ) : CRenderTarget(), m_pWnd( pWnd ) { ASSERT_PTR( m_pWnd ); }

		bool Resize( const SIZE& clientSize );
		bool Resize( void );

		// IRenderHostWindow interface
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const { return m_pWndRenderTarget; }
		virtual CWnd* GetWindow( void ) const { return m_pWnd; }
		virtual bool CanRender( void ) const;

		// IDeviceComponent interface
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );
	private:
		CWnd* m_pWnd;
		CComPtr<ID2D1HwndRenderTarget> m_pWndRenderTarget;		// self-encapsulated, released on device loss
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

		bool Resize( const RECT& subRect );

		// IRenderHostWindow interface
		virtual ID2D1RenderTarget* GetRenderTarget( void ) const { return m_pDcRenderTarget; }
		virtual CWnd* GetWindow( void ) const { return safe_ptr( m_pDC->GetWindow() ); }

		// IDeviceComponent interface
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );
	private:
		CDC* m_pDC;
		CComPtr<ID2D1DCRenderTarget> m_pDcRenderTarget;			// self-encapsulated, released on device loss
	};
}


#endif // ImagingDirect2D_h
