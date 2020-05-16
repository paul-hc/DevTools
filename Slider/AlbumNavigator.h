#ifndef AlbumNavigator_h
#define AlbumNavigator_h
#pragma once

#include "utl/UI/ImagePathKey.h"
#include "AlbumNavigator_fwd.h"


class CAlbumImageView;
class CImagesModel;
class CSlideData;
class CWicImage;


class CAlbumNavigator		// liniarizes navigation within multi-frame positions
{
public:
	CAlbumNavigator( const CAlbumImageView* pAlbumView );

	bool CanNavigate( nav::Navigate navigate, bool multiFrame = true ) const;
	nav::TIndexFramePosPair GetNavigateInfo( nav::Navigate navigate, bool multiFrame = true ) const;

	nav::TIndexFramePosPair GetCurrentInfo( void ) const { return nav::TIndexFramePosPair( m_navigPos, m_imageInfo.m_framePos ); }
	fs::ImagePathKey MakePathKey( const nav::TIndexFramePosPair& infoPair ) const;

	static bool IsAtLimit( const CAlbumImageView* pAlbumView );
private:
	CWicImage* AcquireImageAt( size_t index ) const;

	bool IsAtLimit( void ) const;
	int GetLastNavigPos( void ) const { ASSERT( m_navigCount != 0 ); return m_navigCount - 1; }
private:
	struct CImageInfo
	{
		CImageInfo( const CWicImage* pImage );

		bool IsValidImage( void ) const { return m_frameCount != 0; }
		bool IsMultiFrameImage( void ) const { return !m_isAnimated && m_frameCount > 1; }
		UINT GetLastFramePos( void ) const { ASSERT( IsMultiFrameImage() ); return m_frameCount - 1; }
	public:
		bool m_isAnimated;
		UINT m_framePos;
		UINT m_frameCount;
	};
private:
	const CImagesModel* m_pImagesModel;
	const CSlideData* m_pSlideData;
	CImageInfo m_imageInfo;
	int m_navigPos;
	int m_navigCount;

	//int m_currIndex;
};


#endif // AlbumNavigator_h
