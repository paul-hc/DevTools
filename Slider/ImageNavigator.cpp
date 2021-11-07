
#include "stdafx.h"
#include "ImageNavigator.h"
#include "AlbumImageView.h"
#include "AlbumDoc.h"
#include "FileAttr.h"
#include "utl/ContainerUtilities.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNavigatorBase implementation

CNavigatorBase::CNavigatorBase( const CWicImage* pCurrImage )
	: m_imageInfo( pCurrImage )
	, m_wrapMode( false )
{
}

CNavigatorBase::CImageInfo::CImageInfo( const CWicImage* pImage )
{
	if ( pImage != NULL )
	{
		m_isAnimated = pImage->IsAnimated();
		m_framePos = pImage->GetFramePos();
		m_frameCount = pImage->GetFrameCount();
	}
	else
	{
		m_isAnimated = false;
		m_framePos = m_frameCount = 0;
	}
}


// CImageFrameNavigator implementation

CImageFrameNavigator::CImageFrameNavigator( const CWicImage* pCurrImage )
	: CNavigatorBase( pCurrImage )
	, m_pCurrImage( pCurrImage )
{
}

bool CImageFrameNavigator::CanNavigate( nav::Navigate navigate, UINT step /*= 1*/ ) const
{
	UINT newFramePos = GetNavigateFramePos( navigate, step );

	return newFramePos != m_imageInfo.m_framePos;
}

UINT CImageFrameNavigator::GetNavigateFramePos( nav::Navigate navigate, UINT step /*= 1*/ ) const
{
	UINT newFramePos = m_imageInfo.m_framePos;

	if ( m_imageInfo.IsValidImage() )
		switch ( navigate )
		{
			case nav::Previous:
			case nav::Next:
				// liniarize navigation within multi-frame positions
				if ( m_imageInfo.IsMultiFrameImage() )
					utl::AdvancePos( newFramePos, m_imageInfo.m_frameCount, m_wrapMode, nav::Next == navigate, step );
				break;
			case nav::First:
				newFramePos = 0;
				break;
			case nav::Last:
				newFramePos = m_imageInfo.GetLastFramePos();
				break;
		}

	return newFramePos;
}

fs::TImagePathKey CImageFrameNavigator::MakePathKey( UINT framePos ) const
{
	return fs::TImagePathKey( m_pCurrImage->GetImagePath(), framePos );
}


// CAlbumNavigator implementation

CAlbumNavigator::CAlbumNavigator( const CAlbumImageView* pAlbumView )
	: CNavigatorBase( safe_ptr( pAlbumView )->GetImage() )
	, m_pImagesModel( &safe_ptr( pAlbumView )->GetDocument()->GetModel()->GetImagesModel() )
	, m_pSlideData( &pAlbumView->GetSlideData() )
	, m_navigPos( m_pSlideData->GetCurrentIndex() )
	, m_navigCount( static_cast<int>( m_pImagesModel->GetFileAttrs().size() ) )
{
	ASSERT( m_imageInfo.m_framePos == m_pSlideData->GetCurrentNavPos().second );	// consistent frame-pos?
	m_wrapMode = m_pSlideData->m_wrapMode;
}

bool CAlbumNavigator::CanNavigate( nav::Navigate navigate, UINT step /*= 1*/ ) const
{
	nav::TIndexFramePosPair currentInfo = GetCurrentInfo();
	nav::TIndexFramePosPair newNavigInfo = GetNavigateInfo( navigate, step );

	return newNavigInfo != currentInfo;
}

nav::TIndexFramePosPair CAlbumNavigator::GetNavigateInfo( nav::Navigate navigate, UINT step /*= 1*/ ) const
{
	nav::TIndexFramePosPair navigInfo = GetCurrentInfo();

	if ( m_navigCount != 0 )
		switch ( navigate )
		{
			case nav::Previous:
			case nav::Next:
				// liniarize navigation within multi-frame positions
				if ( m_imageInfo.IsMultiFrameImage() )
				{
					utl::AdvancePos( navigInfo.second, m_imageInfo.m_frameCount, false, nav::Next == navigate, step );

					if ( navigInfo.second != m_imageInfo.m_framePos )		// current frame changed?
						return navigInfo;			// we're done navigating at frame level
				}

				utl::AdvancePos( navigInfo.first, m_navigCount, m_wrapMode, nav::Next == navigate, static_cast<int>( step ) );
				navigInfo.second = 0;

				if ( navigInfo.first != m_navigPos )						// current index changed?
					if ( nav::Previous == navigate )						// navigated to previous image?
						if ( CWicImage* pPrevImage = AcquireImageAt( navigInfo.first ) )
						{
							CImageInfo prevImageInfo( pPrevImage );
							if ( prevImageInfo.IsMultiFrameImage() )
								navigInfo.second = prevImageInfo.GetLastFramePos();			// liniarize backwards: position current frame to the last frame
						}
				break;
			case nav::First:
				navigInfo.first = 0;
				navigInfo.second = 0;
				break;
			case nav::Last:
				navigInfo.first = m_navigCount - 1;
				navigInfo.second = 0;

				if ( CWicImage* pLastImage = AcquireImageAt( navigInfo.first ) )
				{
					CImageInfo lastImageInfo( pLastImage );
					if ( lastImageInfo.IsMultiFrameImage() )
						navigInfo.second = lastImageInfo.GetLastFramePos();				// liniarize end: position current frame to the last frame
				}
				break;
		}

	return navigInfo;
}

fs::TImagePathKey CAlbumNavigator::MakePathKey( const nav::TIndexFramePosPair& infoPair ) const
{
	return fs::TImagePathKey( m_pImagesModel->GetFileAttrAt( infoPair.first )->GetPathKey().first, infoPair.second );
}

CWicImage* CAlbumNavigator::AcquireImageAt( size_t index ) const
{
	return CAlbumDoc::AcquireImage( m_pImagesModel->GetFileAttrAt( index )->GetPathKey() );
}

bool CAlbumNavigator::IsAtLimit( void ) const
{
	if ( 0 == m_navigCount )
		return true;

	if ( 0 == m_navigPos )
		if ( !m_imageInfo.IsMultiFrameImage() || 0 == m_imageInfo.m_framePos )
			return true;

	if ( GetLastNavigPos() == m_navigPos )
		if ( !m_imageInfo.IsMultiFrameImage() || m_imageInfo.GetLastFramePos() == m_imageInfo.m_framePos )
			return true;

	return false;
}

bool CAlbumNavigator::IsAtLimit( const CAlbumImageView* pAlbumView )
{
	CAlbumNavigator navigator( pAlbumView );		// re-evaluate after navigation
	return navigator.IsAtLimit();
}
