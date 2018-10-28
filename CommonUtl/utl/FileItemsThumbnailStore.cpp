
#include "stdafx.h"
#include "FileItemsThumbnailStore.h"
#include "Thumbnailer.h"
#include "ImageStore.h"
#include "BaseApp.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileItemsThumbnailStore implementation

CFileItemsThumbnailStore::CFileItemsThumbnailStore( void )
	: m_pThumbnailer( new CThumbnailer )
{
	app::GetGlobalResources()->GetSharedResources().AddAutoPtr( &m_pThumbnailer );		// will release the shared thumbnailer in ExitInstance()
	m_pThumbnailer->SetOptimizeExtractIcons();					// for more accurate icon scaling that favours the best fitting image size present
}

CFileItemsThumbnailStore::~CFileItemsThumbnailStore()
{
}

CFileItemsThumbnailStore& CFileItemsThumbnailStore::Instance( void )
{
	static CFileItemsThumbnailStore s_thumbnailStore;
	return s_thumbnailStore;
}


// CCustomDrawImager implementation

CCustomDrawImager::CCustomDrawImager( ui::ICustomImageDraw* pRenderer, const CSize& smallImageSize, const CSize& largeImageSize )
	: m_pRenderer( pRenderer )
	, m_ctrlDrivesBoundsSize( smallImageSize != CSize( 0, 0 ) ? true : false )				// image-lists size drive thumbnailer bounds size?
	, m_smallImageSize( m_ctrlDrivesBoundsSize ? smallImageSize : m_pRenderer->GetItemImageSize( ui::ICustomImageDraw::SmallImage ) )
	, m_largeImageSize( m_ctrlDrivesBoundsSize ? largeImageSize : m_pRenderer->GetItemImageSize( ui::ICustomImageDraw::LargeImage ) )
	, m_transpImgIndex( -1 )
{
	ASSERT_PTR( m_pRenderer );
	InitImageLists();
}

void CCustomDrawImager::InitImageLists( void )
{
	REQUIRE( !IsInit() );

	REQUIRE( m_smallImageSize.cx > 0 && m_smallImageSize.cy > 0 );
	REQUIRE( m_largeImageSize.cx > 0 && m_largeImageSize.cy > 0 );
	ASSERT_NULL( m_smallImageList.GetSafeHandle() );
	ASSERT_NULL( m_largeImageList.GetSafeHandle() );

	m_smallImageList.Create( m_smallImageSize.cx, m_smallImageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );
	m_largeImageList.Create( m_largeImageSize.cx, m_largeImageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );

	// add transparent image entry to image lists
	const CIcon* pTranspIcon = CImageStore::SharedStore()->RetrieveIcon( ID_TRANSPARENT );
	ASSERT_PTR( pTranspIcon );

	m_transpImgIndex = m_smallImageList.Add( pTranspIcon->GetHandle() );
	m_largeImageList.Add( pTranspIcon->GetHandle() );
}

bool CCustomDrawImager::UpdateImageSize( ui::ICustomImageDraw::ImageType imgSize )
{
	return
		m_ctrlDrivesBoundsSize &&
		m_pRenderer->SetItemImageSize( ui::ICustomImageDraw::SmallImage == imgSize ? m_smallImageSize : m_largeImageSize );
}
