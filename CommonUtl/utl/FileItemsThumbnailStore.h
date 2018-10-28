#ifndef FileItemsThumbnailStore_h
#define FileItemsThumbnailStore_h
#pragma once

#include "Image_fwd.h"


class CThumbnailer;


// shared thumbnail store for file/folder items that works with with shell icons and image thumbnails automatically
//
class CFileItemsThumbnailStore
{
	CFileItemsThumbnailStore( void );
	~CFileItemsThumbnailStore();
public:
	static CFileItemsThumbnailStore& Instance( void );

	CThumbnailer* GetThumbnailer( void ) const { return safe_ptr( m_pThumbnailer.get() ); }
private:
	std::auto_ptr< CThumbnailer > m_pThumbnailer;
};


// Stores small/large image lists to be assigned to a list or tree contorl, with one transparent image placeholder at m_transpImgIndex (=0).
// Actual thumbnails are custom drawn on top of the transparent image.
//
struct CCustomDrawImager
{
	CCustomDrawImager( ui::ICustomImageDraw* pRenderer, const CSize& smallImageSize, const CSize& largeImageSize );

	bool IsInit( void ) const { return m_transpImgIndex != -1; }
	bool UpdateImageSize( ui::ICustomImageDraw::ImageType imgSize );
private:
	void InitImageLists( void );
public:
	ui::ICustomImageDraw* m_pRenderer;					// thumbnailer
	const bool m_ctrlDrivesBoundsSize;					// true if thumbnailer bounds size is driven by the list control; otherwise the thumbnailer drives image sizes
	const CSize m_smallImageSize, m_largeImageSize;
	CImageList m_smallImageList, m_largeImageList;

	int m_transpImgIndex;					// pos of the transparent entry in the image list
};


#endif // FileItemsThumbnailStore_h
