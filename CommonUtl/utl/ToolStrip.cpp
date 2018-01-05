
#include "stdafx.h"
#include "ToolStrip.h"
#include "ImageStore.h"
#include "ResourceData.h"
#include "resource.h"

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


// CToolStrip implementation

CToolStrip::~CToolStrip()
{
}

void CToolStrip::Clear( void )
{
	m_imageSize.cx = m_imageSize.cy = 0;
	m_buttonIds.clear();
	m_pImageList.reset();
}

int CToolStrip::GetImageCount( void ) const
{
	// image count: (buttons-separators)
	return static_cast< int >( m_buttonIds.size() - std::count( m_buttonIds.begin(), m_buttonIds.end(), ID_SEPARATOR ) );
}

CImageList* CToolStrip::EnsureImageList( void )
{
	if ( !HasImages() )
		if ( CImageStore::HasSharedStore() )
		{
			std::auto_ptr< CImageList > pImageList( new CImageList );
			int imageCount = CImageStore::GetSharedStore()->AddToImageList( *pImageList, &m_buttonIds.front(), m_buttonIds.size(), m_imageSize );
			if ( imageCount == GetImageCount() )			// all buttons images found
				m_pImageList.reset( pImageList.release() );
		}

	return m_pImageList.get();
}

bool CToolStrip::LoadStrip( UINT toolStripId, COLORREF transpColor /*= color::Auto*/ )
{
	ASSERT_PTR( toolStripId );

	CResourceData toolbarData( MAKEINTRESOURCE( toolStripId ), RT_TOOLBAR );
	if ( !toolbarData.IsValid() )
		return false;

	const res::CToolbarInfo* pData = toolbarData.GetResource< res::CToolbarInfo >();
	ASSERT( 1 == pData->m_version );

	m_buttonIds.assign( pData->m_buttonIds, pData->m_buttonIds + pData->m_itemCount );
	m_imageSize = CSize( pData->m_width, pData->m_height );		// button size is known, ready to load the bitmap

	int imageCount = GetImageCount();
	if ( imageCount != 0 )
	{
		std::auto_ptr< CImageList > pImageList( new CImageList );
		if ( res::LoadImageList( *pImageList, toolStripId, imageCount, m_imageSize, transpColor ) )
			m_pImageList.reset( pImageList.release() );
	}
	return imageCount != 0;
}

CToolStrip& CToolStrip::AddButton( UINT buttonId, CIconId iconId /*= CIconId( UseButtonId )*/ )
{
	switch ( iconId.m_id )
	{
		case UseSharedImage:
			if ( const CIcon* pSharedIcon = CImageStore::GetSharedStore()->RetrieveIcon( CIconId( buttonId, iconId.m_stdSize ) ) )
				return AddButton( buttonId, pSharedIcon->GetHandle() );

			ASSERT( false );
			return *this;
		case NullIconId:
			return AddButton( buttonId, (HICON)NULL );
		case UseButtonId:
			iconId.m_id = buttonId;
			break;
	}

	if ( const CIcon* pSharedIcon = CImageStore::GetSharedStore()->RetrieveIcon( iconId ) )
		return AddButton( buttonId, pSharedIcon->GetHandle() );			// use shared icon

	return AddButton( buttonId, CIcon( res::LoadIcon( iconId ) ).GetHandle() );
}

CToolStrip& CToolStrip::AddButton( UINT buttonId, HICON hIcon )
{
	if ( hIcon != NULL )
	{
		if ( NULL == m_pImageList.get() )
		{
			if ( 0 == m_imageSize.cx && 0 == m_imageSize.cy )
			{
				CIconInfo iconInfo( hIcon );
				m_imageSize = iconInfo.m_size;
			}
			m_pImageList.reset( new CImageList );
			VERIFY( m_pImageList->Create( m_imageSize.cx, m_imageSize.cy, ILC_COLOR32, 0, 5 ) );
		}
		VERIFY( m_pImageList->Add( hIcon ) >= 0 );
	}
	else if ( buttonId != ID_SEPARATOR )
	{
		// (*) SPECIAL CASE: toolbar buttons that use controls must have a placeholder image associated; otherwise the image list gets shifted completely;
		// To get rid of this warning, register a command alias for { buttonId, ID_EDIT_DETAILS }
		//
		TRACE( _T(" ** Missing image for button id %d (hex=0x%04X) - using the placeholder image ID_EDIT_DETAILS **\n"), buttonId, buttonId );
		return AddButton( buttonId, CIconId( ID_EDIT_DETAILS ) );
	}

	m_buttonIds.push_back( buttonId );
	return *this;
}

void CToolStrip::AddButtons( const UINT buttonIds[], size_t buttonCount, IconStdSize iconStdSize /*= SmallIcon*/ )
{
	ASSERT( buttonIds != NULL && buttonCount != 0 );

	for ( unsigned int i = 0; i != buttonCount; ++i )
		AddButton( buttonIds[ i ], CIconId( (UINT)UseButtonId, iconStdSize ) );
}

void CToolStrip::RegisterButtons( CImageStore* pImageStore )
{
	if ( NULL == pImageStore )
		pImageStore = CImageStore::SharedStore();

	ASSERT( IsValid() && HasImages() );
	pImageStore->RegisterButtonImages( *m_pImageList, &m_buttonIds.front(), m_buttonIds.size(), &m_imageSize );
}

void CToolStrip::RegisterStripButtons( UINT toolStripId, COLORREF transpColor /*= color::Auto*/, CImageStore* pImageStore /*= NULL*/ )
{
	CToolStrip strip;
	VERIFY( strip.LoadStrip( toolStripId, transpColor ) );
	strip.RegisterButtons( pImageStore );
}
