#ifndef ImageNavigator_h
#define ImageNavigator_h
#pragma once

#include "utl/UI/ImagePathKey.h"
#include "ImageNavigator_fwd.h"


class CWicImage;


abstract class CNavigatorBase
{
protected:
	CNavigatorBase( const CWicImage* pCurrImage );

	struct CImageInfo
	{
		CImageInfo( const CWicImage* pImage );

		bool IsValidImage( void ) const { return m_frameCount != 0; }
		bool IsMultiFrameImage( void ) const { return !m_isAnimated && m_frameCount > 1; }
		UINT GetLastFramePos( void ) const { return m_frameCount - 1; }
	public:
		bool m_isAnimated;
		UINT m_framePos;
		UINT m_frameCount;
	};
protected:
	CImageInfo m_imageInfo;
	bool m_wrapMode;
};


// single-image navigation within multi-frame positions
class CImageFrameNavigator : public CNavigatorBase
{
public:
	CImageFrameNavigator( const CWicImage* pCurrImage );

	bool IsStaticMultiFrameImage( void ) const { return !m_imageInfo.m_isAnimated && m_imageInfo.IsMultiFrameImage(); }
	UINT GetFramePos( void ) const { return m_imageInfo.m_framePos; }
	UINT GetFrameCount( void ) const { return m_imageInfo.m_frameCount; }

	bool CanNavigate( nav::Navigate navigate, UINT step = 1 ) const;
	UINT GetNavigateFramePos( nav::Navigate navigate, UINT step = 1 ) const;

	fs::TImagePathKey MakePathKey( UINT framePos ) const;
private:
	const CWicImage* m_pCurrImage;		// local scope object, ok to store the image
};


class CAlbumImageView;
class CImagesModel;
class CSlideData;


// album navigation, that liniarizes navigation within multi-frame positions
class CAlbumNavigator : private CNavigatorBase
{
public:
	CAlbumNavigator( const CAlbumImageView* pAlbumView );

	bool CanNavigate( nav::Navigate navigate, UINT step = 1 ) const;
	nav::TIndexFramePosPair GetNavigateInfo( nav::Navigate navigate, UINT step = 1 ) const;

	nav::TIndexFramePosPair GetCurrentInfo( void ) const { return nav::TIndexFramePosPair( m_navigPos, m_imageInfo.m_framePos ); }
	fs::TImagePathKey MakePathKey( const nav::TIndexFramePosPair& infoPair ) const;

	static bool IsAtLimit( const CAlbumImageView* pAlbumView );
protected:
	virtual CWicImage* AcquireImageAt( size_t index ) const;
	virtual bool IsAtLimit( void ) const;
private:
	int GetLastNavigPos( void ) const { ASSERT( m_navigCount != 0 ); return m_navigCount - 1; }
private:
	const CImagesModel* m_pImagesModel;
	const CSlideData* m_pSlideData;
	int m_navigPos;
	int m_navigCount;
};


#endif // ImageNavigator_h
