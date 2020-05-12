#ifndef ImageZoomViewD2D_h
#define ImageZoomViewD2D_h
#pragma once

#include "utl/FlexPath.h"
#include "BaseZoomView.h"
#include "ImagingDirect2D.h"
#include "WindowTimer.h"


class CWicImage;


namespace ui
{
	struct CImageFileDetails
	{
		CImageFileDetails( void ) { Reset(); }

		void Reset( const CWicImage* pImage = NULL );

		bool IsValid( void ) const { return !m_filePath.IsEmpty(); }
		bool HasNavigInfo( void ) const { return m_navigCount > 1; }
		double GetMegaPixels( void ) const;
	public:
		fs::CFlexPath m_filePath;
		bool m_isAnimated;
		UINT m_framePos;
		UINT m_frameCount;
		UINT m_fileSize;
		CSize m_dimensions;

		UINT m_navigPos;
		UINT m_navigCount;
	};
}


class CImageZoomViewD2D;


namespace dw { class CImageInfoText; }


namespace d2d
{
	class CAnimatedFrameComposer;
	class CFrameFacet;


	// Owned by the view that displays the STILL/ANIMATED image using a timer and Direct 2D rendering.
	//
	class CImageRenderTarget : public CWindowRenderTarget
	{
		enum { AnimateTimer = 321 };
	public:
		CImageRenderTarget( CImageZoomViewD2D* pZoomView );
		~CImageRenderTarget();

		void SetAccentFrameColor( COLORREF accentFrameColor );

		CFrameFacet* GetAccentFrame( void ) { return m_pAccentFrame.get(); }

		// base overrides
		virtual void DiscardResources( void );
		virtual bool CreateResources( void );
		virtual void StartAnimation( UINT frameDelay );
		virtual void StopAnimation( void );

		virtual void DrawBitmap( const CViewCoords& coords );		// draw still or animated bitmap
		virtual void PreDraw( const CViewCoords& coords );
		virtual void PostDraw( const CViewCoords& coords );

		bool DrawImage( const CViewCoords& coords );

		// animation timer
		bool IsAnimEvent( UINT_PTR eventId ) const { return m_pAnimComposer.get() != NULL && m_animTimer.IsHit( eventId ); }
		void HandleAnimEvent( void );
	private:
		void SetupCurrentImage( void );
	private:
		CWicImage* GetImage( void ) const;
	private:
		CImageZoomViewD2D* m_pZoomView;
		COLORREF m_accentFrameColor;
		std::auto_ptr< CAnimatedFrameComposer > m_pAnimComposer;	// animated frame composition
		CWindowTimer m_animTimer;

		// additional drawing resources
		std::auto_ptr< dw::CImageInfoText > m_pImageInfoText;		// drawn at the bottom
		std::auto_ptr< CFrameFacet > m_pAccentFrame;				// frame drawn when view is focused
	};
}


// scroll view with zomming that displays still or animated WIC images using Direct 2D rendering
//
abstract class CImageZoomViewD2D : public CBaseZoomView
{
protected:
	CImageZoomViewD2D( void );
	virtual ~CImageZoomViewD2D();
public:
	// overrideables
	virtual CWicImage* GetImage( void ) const = 0;
	virtual void QueryImageFileDetails( ui::CImageFileDetails& rImageFileDetails ) const = 0;
	virtual bool IsAccented( void ) const;		// typically colour of the frame when focused

	d2d::CDrawBitmapTraits& GetDrawParams( void ) { return m_drawTraits; }
	d2d::CImageRenderTarget* GetImageRenderTarget( void ) { return m_pImageRT.get(); }
protected:
	// base overrides
	virtual CSize GetSourceSize( void ) const;

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
