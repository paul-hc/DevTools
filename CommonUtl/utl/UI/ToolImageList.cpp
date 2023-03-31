
#include "pch.h"
#include "ToolImageList.h"
#include "Icon.h"
#include "ImageStore.h"
#include "ResourceData.h"
#include "utl/AppTools.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace res
{
	struct CToolbarInfo
	{
		WORD m_version;
		WORD m_width;
		WORD m_height;
		WORD m_itemCount;
		WORD m_buttonIds[ 1 ];		// m_itemCount
	};
}


// CToolImageList implementation

CToolImageList::CToolImageList( IconStdSize iconStdSize /*= SmallIcon*/ )
	: m_imageSize( iconStdSize )
{
}

CToolImageList::CToolImageList( const UINT buttonIds[], size_t count, IconStdSize iconStdSize /*= SmallIcon*/ )
	: m_imageSize( iconStdSize )
	, m_buttonIds( buttonIds, buttonIds + count )
{
}

CToolImageList::~CToolImageList()
{
}

void CToolImageList::Clear( void )
{
	m_imageSize.Reset();
	m_buttonIds.clear();
	m_pImageList.reset();
}

CImageList* CToolImageList::CreateImageList( int countOrGrowBy /*= -5*/ )
{
	m_pImageList.reset( new CImageList() );
	gdi::CreateImageList( *m_pImageList, GetImageSize(), countOrGrowBy );
	return m_pImageList.get();
}

CImageList* CToolImageList::EnsureImageList( void )
{
	if ( HasButtons() && !HasImages() )
		if ( !LoadButtonImages() )
			ASSERT( false );

	return m_pImageList.get();
}

void CToolImageList::ResetImageList( CImageList* pImageList )
{
	m_pImageList.reset( pImageList );

	if ( pImageList != nullptr )
	{
		ENSURE( GetImageCount() == pImageList->GetImageCount() );
		m_imageSize.Reset( gdi::GetImageIconSize( *pImageList ) );
	}
}

int CToolImageList::GetImageCount( void ) const
{
	return static_cast<int>( m_buttonIds.size() - std::count( m_buttonIds.begin(), m_buttonIds.end(), (UINT)ID_SEPARATOR ) );		// image count: buttons minus separators
}

int CToolImageList::EvalButtonCount( const UINT buttonIds[], size_t count )
{
	return static_cast<int>( count - std::count( buttonIds, buttonIds + count, (UINT)ID_SEPARATOR ) );		// image count: buttons minus separators
}

size_t CToolImageList::FindButtonPos( UINT buttonId ) const
{
	return utl::FindPos( m_buttonIds, buttonId );
}

bool CToolImageList::LoadToolbar( UINT toolBarId, COLORREF transpColor /*= color::Auto*/ )
{
	ASSERT( toolBarId != 0 );

	CResourceData toolbarData( MAKEINTRESOURCE( toolBarId ), RT_TOOLBAR );
	if ( !toolbarData.IsValid() )
		return false;

	const res::CToolbarInfo* pData = toolbarData.GetResource<res::CToolbarInfo>();
	ASSERT( 1 == pData->m_version );

	m_buttonIds.assign( pData->m_buttonIds, pData->m_buttonIds + pData->m_itemCount );
	m_imageSize.Reset( CSize( pData->m_width, pData->m_height ) );		// button size is known, ready to load the bitmap

	int imageCount = GetImageCount();
	if ( imageCount != 0 )
	{
		m_pImageList.reset( new CImageList() );
		VERIFY( imageCount == res::LoadImageListDIB( *m_pImageList, toolBarId, transpColor ) );
		ASSERT( m_imageSize.GetSize() == gdi::GetImageIconSize( *m_pImageList ) );
	}
	return imageCount != 0;
}

bool CToolImageList::LoadIconStrip( UINT iconStripId, const UINT buttonIds[], size_t count )		// create imagelist from icon strip (custom size multi-images) and initialize button IDs
{
	CSize imageSize;

	StoreButtonIds( buttonIds, count );
	m_pImageList.reset( new CImageList() );

	int imageCount = res::LoadImageListIconStrip( m_pImageList.get(), &imageSize, iconStripId );
	m_imageSize.Reset( imageSize );
	return imageCount == GetImageCount();		// all buttons images found?
}

bool CToolImageList::LoadButtonImages( ui::IImageStore* pSrcImageStore /*= ui::GetImageStoresSvc()*/ )
{
	ASSERT_PTR( pSrcImageStore );
	REQUIRE( HasButtons() );

	m_pImageList.reset( new CImageList() );
	int imageCount = pSrcImageStore->BuildImageList( m_pImageList.get(), ARRAY_SPAN_V( m_buttonIds ), GetImageSize() );

	return imageCount == GetImageCount();		// all buttons images found?
}
