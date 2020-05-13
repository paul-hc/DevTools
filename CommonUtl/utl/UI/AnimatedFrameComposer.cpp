
#include "stdafx.h"
#include "AnimatedFrameComposer.h"
#include "ImagingWic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	CAnimatedFrameComposer::CAnimatedFrameComposer( d2d::IRenderHostWindow* pRenderHostWnd, CWicAnimatedImage* pAnimImage )
		: m_pRenderHostWnd( pRenderHostWnd )
		, m_pAnimImage( pAnimImage )
	{
		Reset();

		ASSERT_PTR( m_pRenderHostWnd->GetWindow()->GetSafeHwnd() );
		ASSERT_PTR( m_pAnimImage );
	}

	CAnimatedFrameComposer::~CAnimatedFrameComposer()
	{
	}

	ID2D1RenderTarget* CAnimatedFrameComposer::GetWndRenderTarget( void ) const
	{
		return safe_ptr( m_pRenderHostWnd->GetRenderTarget() );
	}

	void CAnimatedFrameComposer::Reset( void )
	{
		m_framePos = 0;
		m_frameMetadata.Reset();

		m_pFrameComposeRT = NULL;
		m_pRawFrame = NULL;
		m_pSavedFrame = NULL;
		m_pFrameToRender = NULL;
	}

	bool CAnimatedFrameComposer::Create( void )
	{
		// create a bitmap render target used to compose frames; bitmap render targets cannot be resized, so we always recreate it.
		m_pFrameComposeRT = NULL;
		if ( !HR_OK( GetWndRenderTarget()->CreateCompatibleRenderTarget( d2d::ToSizeF( m_pAnimImage->GetGifSize() ), &m_pFrameComposeRT ) ) )
			return false;

		// if we have at least one frame, start playing the animation from the first frame
		if ( m_pAnimImage->GetFrameCount() > 0 )
		{
			ComposeNextFrame();
			m_pRenderHostWnd->GetWindow()->InvalidateRect( FALSE );
		}
		return true;
	}

	bool CAnimatedFrameComposer::StoreRawFrame( void )
	{
		m_pRawFrame = NULL;
		m_pFrameToRender = NULL;
		m_frameMetadata.Reset();

		// create a D2DBitmap from IWICBitmapSource
		if ( CComPtr< IWICBitmapSource > pWicFrame = m_pAnimImage->GetDecoder().ConvertFrameAt( m_framePos ) )			// raw WIC frame
			if ( HR_OK( GetWndRenderTarget()->CreateBitmapFromWicBitmap( pWicFrame, NULL, &m_pRawFrame ) ) )			// raw D2D bitmap
			{
				if ( CComPtr< IWICMetadataQueryReader > pMetadataReader = m_pAnimImage->GetDecoder().GetFrameMetadataAt( m_framePos ) )
					m_frameMetadata.Store( pMetadataReader );		// store the Metadata for the current frame

				return true;
			}

		return false;
	}

	bool CAnimatedFrameComposer::ComposeNextFrame( void )
	{
		m_pRenderHostWnd->StopAnimation();	// delay is no longer valid

		if ( NULL == m_pFrameComposeRT )
			return false;

		bool succeeded = StepFrame();		// compose one frame

		// keep composing frames until we see a frame with delay greater than 0 (0 delay frames are the invisible intermediate frames), or until we have reached the last frame
		while ( succeeded && 0 == m_frameMetadata.m_frameDelay && !IsLastFrame() )
			succeeded = StepFrame();

		// set the timer regardless of whether we succeeded in composing a frame to try our best to continue displaying the animation
		if ( m_pAnimImage->GetFrameCount() > 1 )
			m_pRenderHostWnd->StartAnimation( m_frameMetadata.m_frameDelay );

		return succeeded;
	}

	bool CAnimatedFrameComposer::StepFrame( void )
	{
		return DisposeCurrentFrame() && OverlayNextFrame();
	}

	bool CAnimatedFrameComposer::DisposeCurrentFrame( void )
	{
		switch ( m_frameMetadata.m_frameDisposal )
		{
			case CWicAnimatedImage::DM_Undefined:
			case CWicAnimatedImage::DM_None:
				return true;							// we simply draw on the previous frames, do nothing here
			case CWicAnimatedImage::DM_Background:
				return ClearCurrentFrameArea();			// dispose background: clear the area covered by the current raw frame with background color
			case CWicAnimatedImage::DM_Previous:
				return RestoreSavedFrame();				// dispose previous: restore the previous composed frame first
			default:
				return false;		// invalid disposal method
		}
	}

	bool CAnimatedFrameComposer::OverlayNextFrame( void )
	{
		// load and render the next raw frame into the composed frame render target; this is called after the current frame is disposed.
		if ( !StoreRawFrame() )							// store current frame info
			return false;

		// for disposal 3 method, we would want to save a copy of the current composed frame
		if ( CWicAnimatedImage::DM_Previous == m_frameMetadata.m_frameDisposal )
			if ( !SaveComposedFrame() )
				return false;

		// produce the current frame
		m_pFrameComposeRT->BeginDraw();

		if ( 0 == m_framePos )
			m_pFrameComposeRT->Clear( m_pAnimImage->GetBkgndColor() );		// clear background on first frame

		m_pFrameComposeRT->DrawBitmap( m_pRawFrame, d2d::ToRectF( m_frameMetadata.m_rect ) );

		if ( !HR_OK( m_pFrameComposeRT->EndDraw() ) )
			return false;

		// cache the composed frame to improve performance and avoid decoding/composing this frame in the following animation loops
		ASSERT_NULL( m_pFrameToRender );
		if ( !HR_OK( m_pFrameComposeRT->GetBitmap( &m_pFrameToRender ) ) )
			return false;

		m_framePos = ( ++m_framePos ) % m_pAnimImage->GetFrameCount();		// increment to the next frame index
		return true;
	}

	bool CAnimatedFrameComposer::SaveComposedFrame( void )
	{
		// save the current composed frame (in bitmap render target) into a temporary bitmap
		CComPtr< ID2D1Bitmap > pComposedFrame;
		if ( HR_OK( m_pFrameComposeRT->GetBitmap( &pComposedFrame ) ) )
		{
			// create the temporary bitmap if it hasn't been created yet
			if ( NULL == m_pSavedFrame )
			{
				D2D1_SIZE_U bitmapSize = pComposedFrame->GetPixelSize();
				D2D1_BITMAP_PROPERTIES bitmapProps;
				pComposedFrame->GetDpi( &bitmapProps.dpiX, &bitmapProps.dpiY );
				bitmapProps.pixelFormat = pComposedFrame->GetPixelFormat();

				if ( !HR_OK( m_pFrameComposeRT->CreateBitmap( bitmapSize, bitmapProps, &m_pSavedFrame ) ) )
					return false;
			}
		}

		return HR_OK( m_pSavedFrame->CopyFromBitmap( NULL, pComposedFrame, NULL ) );			// copy the whole bitmap
	}

	bool CAnimatedFrameComposer::RestoreSavedFrame( void )
	{
		CComPtr< ID2D1Bitmap > pFrameToCopyTo;
		if ( m_pSavedFrame != NULL )
			if ( HR_OK( m_pFrameComposeRT->GetBitmap( &pFrameToCopyTo ) ) )
				if ( HR_OK( pFrameToCopyTo->CopyFromBitmap( NULL, m_pSavedFrame, NULL ) ) )		// copy the whole bitmap
					return true;

		return false;
	}

	bool CAnimatedFrameComposer::ClearCurrentFrameArea( void )
	{
		// clear the rectangular area overlaid by the current raw frame in the bitmap render target with background color
		m_pFrameComposeRT->BeginDraw();

		// clip the render target to the size of the raw frame
		m_pFrameComposeRT->PushAxisAlignedClip( d2d::ToRectF( m_frameMetadata.m_rect ), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE );
		m_pFrameComposeRT->Clear( m_pAnimImage->GetBkgndColor() );
		m_pFrameComposeRT->PopAxisAlignedClip();			// remove the clipping

		return SUCCEEDED( m_pFrameComposeRT->EndDraw() );
	}

	RenderResult CAnimatedFrameComposer::DrawBitmap( const CViewCoords& coords, const CBitmapCoords& bmpCoords )
	{
		if ( NULL == m_pFrameComposeRT )
			return RenderError;

		bmpCoords.m_dbmTraits.Draw( GetWndRenderTarget(), m_pFrameToRender, coords.m_contentRect, bmpCoords.m_pSrcBmpRect );
		return RenderDone;
	}

	void CAnimatedFrameComposer::HandleAnimEvent( void )
	{
		ComposeNextFrame();									// display next frame and set a new timer as needed
		m_pRenderHostWnd->GetWindow()->InvalidateRect( FALSE );
	}

} //namespace d2d
