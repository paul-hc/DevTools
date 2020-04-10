#ifndef ImageZoomViewD2D_h
#define ImageZoomViewD2D_h
#pragma once

#include "BaseZoomView.h"
#include "ImagingDirect2D.h"
#include "WindowTimer.h"


class CWicImage;
class CWicAnimatedImage;
class CImageZoomViewD2D;


namespace d2d
{
	class CAnimatedFrameComposer;


	// owned by the view that displays the still or animated image using a timer and Direct 2D rendering
	//
	class CImageRenderTarget : public CWindowRenderTarget
	{
		enum { AnimateTimer = 321 };
	public:
		CImageRenderTarget( CImageZoomViewD2D* pZoomView );
		~CImageRenderTarget();

		// base overrides
		virtual void DiscardResources( void );
		virtual bool CreateResources( void );

		bool DrawImage( const CDrawBitmapTraits& traits );

		// animation timer
		bool IsAnimEvent( UINT_PTR eventId ) const { return m_pAnimComposer.get() != NULL && m_animTimer.IsHit( eventId ); }
		void HandleAnimEvent( void );
	private:
		void SetupCurrentImage( void );
	private:
		CWicImage* GetImage( void ) const;
		CWicAnimatedImage* GetAnimImage( void ) const;
		bool HasAnimatedImage( void );
	private:
		CImageZoomViewD2D* m_pZoomView;
		std::auto_ptr< CAnimatedFrameComposer > m_pAnimComposer;		// animated frame composition
		CWindowTimer m_animTimer;
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
	virtual CWicImage* GetImage( void ) const = 0;

	d2d::CDrawBitmapTraits& GetDrawParams( void ) { return m_drawTraits; }
	d2d::CImageRenderTarget* GetImageRenderTarget( void ) { return m_pImageRT.get(); }
protected:
	// base overrides
	virtual CSize GetSourceSize( void ) const;

	bool IsValidRenderTarget( void ) const { return m_pImageRT.get() != NULL && m_pImageRT->IsValid(); }
	void PrintImageGdi( CDC* pPrnDC, CWicImage* pImage );
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

	DECLARE_MESSAGE_MAP()
};


#endif // ImageZoomViewD2D_h
