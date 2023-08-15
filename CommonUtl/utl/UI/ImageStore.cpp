
#include "pch.h"
#include "ImageStore.h"
#include "Imaging.h"
#include "Dialog_fwd.h"
#include "ThemeItem.h"
#include "ToolImageList.h"
#include "PopupMenus_fwd.h"			// for mfc::RegisterCmdImageAlias()
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include <afxcommandmanager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	struct DeleteMenuBitmaps
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			delete rPair.second.first;
			delete rPair.second.second;
		}
	};
}


// CImageStore implementation

CImageStore::CImageStore( void )
	: m_pMenuItemBkTheme( new CThemeItem( _T("MENU"), MENU_POPUPBACKGROUND, 0 ) )							// item background (opaque)
	, m_pCheckedMenuItemBkTheme( new CThemeItem( _T("MENU"), MENU_POPUPCHECKBACKGROUND, MCB_BITMAP ) )		// checked button background (semi-transparent)
{
	CImageStoresSvc::GetInstance()->PushStore( this );
}

CImageStore::~CImageStore()
{
	Clear();
	CImageStoresSvc::GetInstance()->PopStore( this );
}

void CImageStore::Clear( void )
{
	utl::ClearOwningMapValues( m_iconMap );
	utl::ClearOwningMapValues( m_bitmapMap );
	utl::ClearOwningContainer( m_menuBitmapMap, func::DeleteMenuBitmaps() );
}

CIcon* CImageStore::FindIcon( UINT cmdId, IconStdSize iconStdSize /*= SmallIcon*/ ) const
{
	const TIconKey key( FindAliasIconId( cmdId ), iconStdSize );
	TIconMap::const_iterator itFound = m_iconMap.find( key );
	if ( itFound == m_iconMap.end() )
		return nullptr;

	return itFound->second;
}

void CImageStore::RegisterAlias( UINT cmdId, UINT iconId )
{
	ASSERT( cmdId != 0 && iconId != 0 );
	m_cmdAliasMap[ cmdId ] = iconId;

	mfc::RegisterCmdImageAlias( cmdId, iconId );
}

void CImageStore::RegisterAliases( const ui::CCmdAlias iconAliases[], size_t count )
{
	for ( size_t i = 0; i != count; ++i )
		RegisterAlias( iconAliases[ i ].m_cmdId, iconAliases[ i ].m_imageCmdId );
}

const CIcon* CImageStore::RetrieveIcon( const CIconId& cmdId )
{
	if ( CIcon* pFoundIcon = FindIcon( cmdId.m_id, cmdId.m_stdSize ) )
		return pFoundIcon;

	const TIconKey iconKey( FindAliasIconId( cmdId.m_id ), cmdId.m_stdSize );		// try loading iconId always (not the command alias)
	if ( CIcon* pIcon = CIcon::NewExactIcon( CIconId( iconKey.first, cmdId.m_stdSize ) ) )
	{
		m_iconMap[ iconKey ] = pIcon;
		return pIcon;
	}

	return nullptr;
}

CBitmap* CImageStore::RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor )
{
	const TBitmapKey key( cmdId.m_id, transpColor );
	TBitmapMap::const_iterator itFound = m_bitmapMap.find( key );
	if ( itFound != m_bitmapMap.end() )
		return itFound->second;

	if ( const CIcon* pIcon = RetrieveIcon( cmdId ) )
	{
		ASSERT( pIcon->IsValid() );

		CBitmap* pBitmap = new CBitmap();
		pIcon->MakeBitmap( *pBitmap, transpColor );
		m_bitmapMap[ key ] = pBitmap;
		return pBitmap;
	}
	return nullptr;
}

ui::IImageStore::TBitmapPair CImageStore::RetrieveMenuBitmaps( const CIconId& cmdId )
{
	std::unordered_map<UINT, TBitmapPair>::const_iterator itFound = m_menuBitmapMap.find( cmdId.m_id );
	if ( itFound != m_menuBitmapMap.end() )
		return itFound->second;

	if ( const CIcon* pIcon = RetrieveIcon( cmdId ) )
	{
		ASSERT( pIcon->IsValid() );

		TBitmapPair& rBitmapPair = m_menuBitmapMap[ cmdId.m_id ];
		rBitmapPair.first = RenderMenuBitmap( *pIcon, false );		// unchecked state
		rBitmapPair.second = RenderMenuBitmap( *pIcon, true );		// checked state
		return rBitmapPair;
	}
	return TBitmapPair( nullptr, nullptr );
}

ui::IImageStore::TBitmapPair CImageStore::RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps )
{
	if ( useCheckedBitmaps )
		return RetrieveMenuBitmaps( cmdId );		// use rendered DDBs
	else
	{
		// straight bitmap looks better because the DIB section retains the alpha channel;
		// no image for checked state (will use a check mark)
		return TBitmapPair( RetrieveBitmap( cmdId, GetSysColor( COLOR_MENU ) ), nullptr );
	}
}

CBitmap* CImageStore::RenderMenuBitmap( const CIcon& icon, bool checked ) const
{
	enum { Edge = 2 };

	CSize iconSize = icon.GetSize();
	CSize bitmapSize = iconSize + CSize( Edge * 2, Edge * 2 );

	std::auto_ptr<CBitmap> pMenuBitmap( new CBitmap );
	CWindowDC screenDC( nullptr );
	CDC memDC;
	if ( memDC.CreateCompatibleDC( &screenDC ) )
		if ( pMenuBitmap->CreateCompatibleBitmap( &screenDC, bitmapSize.cx, bitmapSize.cy ) )
		{
			CBitmap* pOldBitmap = memDC.SelectObject( pMenuBitmap.get() );
			CRect buttonRect( 0, 0, bitmapSize.cx, bitmapSize.cy );
			CPoint iconPos( Edge, Edge );

			m_pMenuItemBkTheme->DrawBackground( memDC, buttonRect );
			if ( checked )
				m_pCheckedMenuItemBkTheme->DrawBackground( memDC, buttonRect );

			icon.Draw( memDC, iconPos );

			memDC.SelectObject( pOldBitmap );
			return pMenuBitmap.release();
		}

	return nullptr;
}

void CImageStore::RegisterToolbarImages( UINT toolBarId, COLORREF transpColor /*= color::Auto*/ )
{
	CToolImageList strip;

	VERIFY( strip.LoadToolbar( toolBarId, transpColor ) );
	RegisterButtonImages( strip );
}

void CImageStore::RegisterButtonImages( const CToolImageList& toolImageList )
{
	REQUIRE( toolImageList.IsValid() );
	RegisterButtonImages( *toolImageList.GetImageList(), ARRAY_SPAN_V( toolImageList.GetButtonIds() ), &toolImageList.GetImageSize() );
}

void CImageStore::RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, const CSize* pImageSize /*= nullptr*/ )
{
	ASSERT_PTR( imageList.GetSafeHandle() );
	ASSERT( imageList.GetImageCount() <= (int)buttonCount );		// give or take the separators

	const bool hasAlpha = !gdi::HasMask( imageList, 0 );			// could also use ui::HasAlphaTransparency() - assume all images are the same
	CSize imageSize = pImageSize != nullptr ? *pImageSize : gdi::GetImageIconSize( imageList );
	TIconKey iconKey( 0, ui::LookupIconStdSize( imageSize.cy ) );

	ENSURE( iconKey.second != DefaultSize );

	for ( UINT i = 0, imagePos = 0; i != buttonCount; ++i )
		if ( buttonIds[ i ] != ID_SEPARATOR )			// skip separators
		{
			iconKey.first = buttonIds[ i ];

			TIconMap::const_iterator itFound = m_iconMap.find( iconKey );
			if ( itFound == m_iconMap.end() )
			{
				HICON hIcon = const_cast<CImageList&>( imageList ).ExtractIcon( imagePos );
				ASSERT_PTR( hIcon );
				CIcon* pIcon = new CIcon( hIcon, imageSize );
				pIcon->SetHasAlpha( hasAlpha );

				m_iconMap[ iconKey ] = pIcon;
			}
			++imagePos;
		}
}

void CImageStore::RegisterIcon( UINT cmdId, CIcon* pIcon )
{
	ASSERT( cmdId != 0 );
	ASSERT( pIcon != nullptr && pIcon->IsValid() );

	TIconKey iconKey( cmdId, ui::LookupIconStdSize( pIcon->GetSize().cx ) );

	CIcon*& rpIcon = m_iconMap[ iconKey ];
	delete rpIcon;
	rpIcon = pIcon;
}


// CImageStoresSvc implementation

CImageStoresSvc::CImageStoresSvc( void )
{
}

CImageStoresSvc* CImageStoresSvc::GetInstance( void )
{
	static CImageStoresSvc s_repository;
	return &s_repository;
}

void CImageStoresSvc::PushStore( ui::IImageStore* pImageStore )
{
	ASSERT( !utl::Contains( m_imageStores, pImageStore ) );
	m_imageStores.push_back( pImageStore );
}

void CImageStoresSvc::PopStore( ui::IImageStore* pImageStore )
{
	std::vector<ui::IImageStore*>::iterator itFound = std::find( m_imageStores.begin(), m_imageStores.end(), pImageStore );
	REQUIRE( itFound != m_imageStores.end() );

	m_imageStores.erase( itFound );
}

CIcon* CImageStoresSvc::FindIcon( UINT cmdId, IconStdSize iconStdSize /*= SmallIcon*/ ) const
{
	// reverse iterate so the most recent store has priority (stack top, at the back)
	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
		if ( CIcon* pFoundIcon = (*itStore)->FindIcon( cmdId, iconStdSize ) )
			return pFoundIcon;

	return nullptr;
}

const CIcon* CImageStoresSvc::RetrieveIcon( const CIconId& cmdId )
{
	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
		if ( const CIcon* pFoundIcon = (*itStore)->RetrieveIcon( cmdId ) )
			return pFoundIcon;

	return nullptr;
}

CBitmap* CImageStoresSvc::RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor )
{
	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
		if ( CBitmap* pFoundBitmap = (*itStore)->RetrieveBitmap( cmdId, transpColor ) )
			return pFoundBitmap;

	return nullptr;
}

ui::IImageStore::TBitmapPair CImageStoresSvc::RetrieveMenuBitmaps( const CIconId& cmdId )
{
	TBitmapPair foundPair;

	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
	{
		foundPair = (*itStore)->RetrieveMenuBitmaps( cmdId );
		if ( foundPair.first != nullptr || foundPair.second != nullptr )
			break;
	}

	return foundPair;
}

ui::IImageStore::TBitmapPair CImageStoresSvc::RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps )
{
	TBitmapPair foundPair;

	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
	{
		foundPair = (*itStore)->RetrieveMenuBitmaps( cmdId, useCheckedBitmaps );
		if ( foundPair.first != nullptr || foundPair.second != nullptr )
			break;
	}

	return foundPair;
}
