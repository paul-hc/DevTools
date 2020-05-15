#ifndef ImageZoomViewD2D_h
#define ImageZoomViewD2D_h
#pragma once

#include "BaseZoomView.h"
#include "ImagingDirect2D.h"
#include "WindowTimer.h"
#include "IImageZoomView.h"


namespace d2d
{
	class CAnimatedFrameComposer;
	class CFrameGadget;
	class CImageInfoGadget;


	// Owned by the view that displays the STILL/ANIMATED image using a timer and Direct 2D rendering.
	//
	class CImageRenderTarget : public CWindowRenderTarget
	{
		enum { AnimateTimer = 321 };
	public:
		CImageRenderTarget( ui::IImageZoomView* pImageView );
		~CImageRenderTarget();

		// base overrides
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );
		virtual void StartAnimation( UINT frameDelay );
		virtual void StopAnimation( void );

		virtual void DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords );		// draw still or animated bitmap
		virtual void PreDraw( const CViewCoords& coords );
		virtual bool IsGadgetVisible( const IGadgetComponent* pGadget ) const;

		bool DrawImage( const CViewCoords& coords, const CBitmapCoords& bmpCoords );

		// animation timer
		bool IsAnimEvent( UINT_PTR eventId ) const { return m_pAnimComposer.get() != NULL && m_animTimer.IsHit( eventId ); }
		void HandleAnimEvent( void );
	private:
		CFrameGadget* MakeAccentFrameGadget( void ) const;
		void SetupCurrentImage( void );

		CWicImage* GetImage( void ) const;
	private:
		ui::IImageZoomView* m_pImageView;
		COLORREF m_accentFrameColor;
		std::auto_ptr< CAnimatedFrameComposer > m_pAnimComposer;	// animated frame composition
		CWindowTimer m_animTimer;

		// additional drawing resources
		std::auto_ptr< CImageInfoGadget > m_pImageInfoGadget;		// drawn at the bottom
		std::auto_ptr< CFrameGadget > m_pAccentFrameGadget;			// frame drawn when view is focused
	};
}


// scroll view with zomming that displays still or animated WIC images using Direct 2D rendering
//
abstract class CImageZoomViewD2D : public CBaseZoomView
								 , public ui::IImageZoomView
{
protected:
	CImageZoomViewD2D( void );
	virtual ~CImageZoomViewD2D();
public:
	d2d::CDrawBitmapTraits& GetDrawParams( void ) { return m_drawTraits; }
	d2d::CImageRenderTarget* GetImageRenderTarget( void ) { return m_pImageRT.get(); }
protected:
	// ui::IZoomView interface (partial)
	virtual CSize GetSourceSize( void ) const;

	// ui::IImageZoomView interface (partial)
	virtual ui::IZoomView* GetZoomView( void );

	bool IsValidRenderTarget( void ) const { return m_pImageRT.get() != NULL && m_pImageRT->IsValidTarget(); }
	void PrintImageGdi( CDC* pPrintDC, CWicImage* pImage );
private:
	std::auto_ptr< d2d::CImageRenderTarget > m_pImageRT;
	d2d::CDrawBitmapTraits m_drawTraits;

	// generated stuff
protected:
	virtual void OnDraw( CDC* pDC );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	virtual void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg LRESULT OnDisplayChange( WPARAM bitsPerPixel, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageZoomViewD2D_h
