#ifndef AnimatedFrameComposer_h
#define AnimatedFrameComposer_h
#pragma once

#include "WicAnimatedImage.h"
#include "ImagingDirect2D.h"


namespace d2d
{
	// manages frame composing for animated images in a Direct 2D zoom view
	//
	class CAnimatedFrameComposer
	{
	public:
		CAnimatedFrameComposer( d2d::IRenderHostWindow* pRenderHostWnd, CWicAnimatedImage* pAnimImage );
		~CAnimatedFrameComposer();

		bool UsesImage( CWicImage* pImage ) const { return m_pAnimImage == pImage; }

		void Reset( void );
		bool Create( void );

		RenderResult DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords );
		void HandleAnimEvent( void );
	private:
		bool IsLastFrame( void ) const { return m_framePos == m_pAnimImage->GetFrameCount() - 1; }
		ID2D1RenderTarget* GetWndRenderTarget( void ) const;

		bool StepFrame( void );
		bool ComposeNextFrame( void );
		bool DisposeCurrentFrame( void );
		bool OverlayNextFrame( void );

		bool StoreRawFrame( void );
		bool ClearCurrentFrameArea( void );
		bool SaveComposedFrame( void );
		bool RestoreSavedFrame( void );
	private:
		d2d::IRenderHostWindow* m_pRenderHostWnd;
		CWicAnimatedImage* m_pAnimImage;
		UINT m_framePos;
		CWicAnimatedImage::CFrameMetadata m_frameMetadata;			// metadata of current frame

		CComPtr<ID2D1BitmapRenderTarget> m_pFrameComposeRT;
		CComPtr<ID2D1Bitmap> m_pRawFrame;
		CComPtr<ID2D1Bitmap> m_pSavedFrame;						// the temporary bitmap used for DM_PREVIOUS
		CComPtr<ID2D1Bitmap> m_pFrameToRender;					// cached frame bitmap to be rendered
	};
}


#endif // AnimatedFrameComposer_h
