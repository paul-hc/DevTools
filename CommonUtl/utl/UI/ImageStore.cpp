
#include "pch.h"
#include "ImageStore.h"
#include "IconGroup.h"
#include "Imaging.h"
#include "Dialog_fwd.h"
#include "ThemeItem.h"
#include "ToolImageList.h"
#include "PopupMenus_fwd.h"			// for mfc::RegisterCmdImageAlias()
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include <afxcommandmanager.h>
#include <afxcontextmenumanager.h>

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


namespace utl
{
	// CStripBtnInfoHasher implementation

	size_t CStripBtnInfoHasher::operator()( const ui::CStripBtnInfo& stripBtnInfo ) const
	{
		return utl::GetHashCombine( stripBtnInfo.m_cmdId, stripBtnInfo.m_imagePos );
	}
}


namespace ui
{
	// CToolbarDescr implementation

	CToolbarDescr::CToolbarDescr( UINT toolbarId, const UINT buttonIds[] /*= nullptr*/, size_t buttonCount /*= 0*/ )
		: m_toolbarId( toolbarId )
		, m_toolbarTitle( str::Load( toolbarId ) )
	{
		REQUIRE( m_toolbarId != 0 );
		StoreBtnInfos( buttonIds, buttonCount );
	}

	void CToolbarDescr::StoreBtnInfos( const UINT buttonIds[], size_t buttonCount )
	{
		m_btnInfos.clear();
		m_btnInfos.reserve( buttonCount );

		for ( UINT imagePos = 0; imagePos != buttonCount; ++imagePos )
			if ( buttonIds[ imagePos ] != ID_SEPARATOR )			// skip separators
				m_btnInfos.push_back( ui::CStripBtnInfo( buttonIds[ imagePos ], imagePos ) );
	}

	const ui::CStripBtnInfo* CToolbarDescr::FindBtnInfo( UINT cmdId ) const
	{
		std::vector<ui::CStripBtnInfo>::const_iterator itFound = std::find_if( m_btnInfos.begin(), m_btnInfos.end(), pred::HasCmdId( cmdId ) );
		if ( itFound == m_btnInfos.end() )
			return nullptr;

		return &*itFound;
	}
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
	utl::ClearOwningContainer( m_toolbarDescriptors );
	utl::ClearOwningMapValues( m_iconFramesMap );
	utl::ClearOwningMapValues( m_bitmapMap );
	utl::ClearOwningContainer( m_menuBitmapMap, func::DeleteMenuBitmaps() );
}

CIconGroup* CImageStore::FindIconGroup( UINT cmdId ) const
{
	UINT iconResId = FindAliasIconId( cmdId );
	TIconGroupMap::const_iterator itFound = m_iconFramesMap.find( iconResId );

	if ( itFound == m_iconFramesMap.end() )
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
	UINT iconResId = FindAliasIconId( cmdId.m_id );

	if ( CIcon* pFoundIcon = FindIcon( iconResId, cmdId.m_stdSize ) )
		return pFoundIcon;

	if ( CIcon* pIcon = CIcon::LoadNewExactIcon( CIconId( iconResId, cmdId.m_stdSize ) ) )
	{
		const ui::CIconKey iconKey( iconResId, ui::CIconEntry( pIcon->GetBitsPerPixel(), cmdId.m_stdSize ) );

		MapIcon( iconKey, pIcon );
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

void CImageStore::QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const
{
	rToolbarDescrs.insert( rToolbarDescrs.end(), m_toolbarDescriptors.begin(), m_toolbarDescriptors.end() );
}

void CImageStore::QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const
{
	utl::QueryThat( rToolbarDescrs, m_toolbarDescriptors, pred::HasCmdId( cmdId ) );
}

void CImageStore::QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize /*= AnyIconSize*/ ) const
{
	for ( std::vector<ui::CIconKey>::const_iterator itIconKey = m_iconKeys.begin(); itIconKey != m_iconKeys.end(); ++itIconKey )
		if ( AnyIconSize == iconStdSize || iconStdSize == itIconKey->m_stdSize )
			rIconKeys.push_back( *itIconKey );
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

UINT CImageStore::FindAliasIconId( UINT cmdId ) const
{
	std::unordered_map<UINT, UINT>::const_iterator itFound = m_cmdAliasMap.find( cmdId );

	if ( itFound != m_cmdAliasMap.end() )
		return itFound->second;					// found icon alias for the command
	return cmdId;
}

CIconGroup*& CImageStore::MapIconGroup( CIconGroup* pIconGroup )
{
	ASSERT_PTR( pIconGroup );

	CIconGroup*& rpIconGroup = m_iconFramesMap[ pIconGroup->GetIconResId() ];

	if ( pIconGroup != rpIconGroup )		// a new object?
	{
		if ( rpIconGroup != nullptr )		// replace existing IconGroup?
		{
			for ( size_t pos = 0, frameCount = rpIconGroup->GetSize(); pos != frameCount; ++pos )
			{
				ui::CIconKey iconKey = rpIconGroup->GetIconKeyAt( pos );
				std::vector<ui::CIconKey>::const_iterator itFoundKey = std::find( m_iconKeys.begin(), m_iconKeys.end(), iconKey );

				if ( itFoundKey != m_iconKeys.end() )
				{
					m_iconKeys.erase( itFoundKey );		// remove existing key
					ASSERT( 0 == std::count( m_iconKeys.begin(), m_iconKeys.end(), iconKey ) );		// we shouldn't have any duplicates leftover
				}
			}

			delete rpIconGroup;
		}

		rpIconGroup = pIconGroup;

		for ( size_t pos = 0, frameCount = rpIconGroup->GetSize(); pos != frameCount; ++pos )
			m_iconKeys.push_back( rpIconGroup->GetIconKeyAt( pos ) );
	}

	return rpIconGroup;
}

bool CImageStore::MapIcon( const ui::CIconKey& iconKey, CIcon* pIcon )
{
	if ( CIconGroup* pIconGroup = FindIconGroup( iconKey.m_iconResId ) )
		if ( pIconGroup->AugmentIcon( iconKey, pIcon ) )
			m_iconKeys.push_back( iconKey );
		else
		{
			std::vector<ui::CIconKey>::iterator itFoundKey = std::find( m_iconKeys.begin(), m_iconKeys.end(), iconKey );

			ASSERT( itFoundKey != m_iconKeys.end() );						// ensure vector consistent with the hash map
			std::rotate( itFoundKey, itFoundKey + 1, m_iconKeys.end() );	// move it at the back (as most recent)
			return false;			// icon replaced
		}
	else
	{	// add new icon group
		pIconGroup = new CIconGroup();

		pIconGroup->SetIconResId( iconKey.m_iconResId );
		pIconGroup->AddIcon( iconKey, pIcon );

		MapIconGroup( pIconGroup );
	}

	return true;	// icon added
}

void CImageStore::RegisterToolbarImages( UINT toolbarId, COLORREF transpColor /*= color::Auto*/, bool addMfcToolBarImages /*= false*/ )
{
	CToolImageList strip;

	VERIFY( strip.LoadToolbar( toolbarId, transpColor ) );
	RegisterButtonImages( strip );
	m_toolbarDescriptors.push_back( new ui::CToolbarDescr( toolbarId, ARRAY_SPAN_V( strip.GetButtonIds() ) ) );

	if ( afxContextMenuManager != nullptr || addMfcToolBarImages )
		CMFCToolBar::AddToolBarForImageCollection( toolbarId );		// also load the MFC control bar images for the toolbar
}

void CImageStore::RegisterButtonImages( const CToolImageList& toolImageList )
{
	REQUIRE( toolImageList.IsValid() );
	RegisterButtonImages( *toolImageList.GetImageList(), ARRAY_SPAN_V( toolImageList.GetButtonIds() ), toolImageList.HasAlpha(), &toolImageList.GetImageSize() );
}

void CImageStore::RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, bool hasAlpha, const CSize* pImageSize /*= nullptr*/ )
{
	ASSERT_PTR( imageList.GetSafeHandle() );
	ASSERT( imageList.GetImageCount() <= (int)buttonCount );			// give or take the separators

	//const bool hasAlpha = hasAlpha || !gdi::HasMask( imageList, 0 );	// could also use ui::HasAlphaTransparency() - assume all images are the same
	CSize imageSize = pImageSize != nullptr ? *pImageSize : gdi::GetImageIconSize( imageList );
	ui::CIconKey iconKey( 0, ui::CIconEntry( ILC_COLOR32, imageSize ) );

	ENSURE( iconKey.m_stdSize != DefaultSize );

	for ( UINT i = 0, imagePos = 0; i != buttonCount; ++i )
		if ( buttonIds[ i ] != ID_SEPARATOR )			// skip separators
		{
			iconKey.m_iconResId = buttonIds[ i ];

			CIcon* pExistingIcon = FindIcon( iconKey.m_iconResId, iconKey.m_stdSize, iconKey.m_bitsPerPixel );		// find exact matching icon

			if ( nullptr == pExistingIcon )		// not already loaded?
			{
				HICON hIcon = const_cast<CImageList&>( imageList ).ExtractIcon( imagePos );
				ASSERT_PTR( hIcon );

				CIcon* pIcon = new CIcon( hIcon, imageSize );
				pIcon->SetHasAlpha( hasAlpha );

				MapIcon( iconKey, pIcon );
			}
			++imagePos;
		}
}

void CImageStore::RegisterIcon( UINT cmdId, CIcon* pIcon )
{
	ASSERT( cmdId != 0 );
	ASSERT( pIcon != nullptr && pIcon->IsValid() );

	// TODO: refine by getting the exact BPP
	ui::CIconKey iconKey( cmdId, ui::CIconEntry( pIcon->HasAlpha() ? ILC_COLOR32 : ILC_COLOR24, pIcon->GetSize() ) );

	MapIcon( iconKey, pIcon );
}

CIcon* CImageStore::RegisterLoadIcon( const CIconId& iconId )
{	// load icon from resources
	CIcon* pIcon = CIcon::LoadNewIcon( iconId );

	if ( pIcon != nullptr )
		RegisterIcon( iconId.m_id, pIcon );

	return pIcon;
}

CIconGroup* CImageStore::RegisterLoadIconGroup( UINT iconId )
{
	std::auto_ptr<CIconGroup> pIconGroupRes( new CIconGroup() );

	if ( pIconGroupRes->LoadAllIcons( iconId ) != 0 )
		return MapIconGroup( pIconGroupRes.release() );		// pass the ownership

	return nullptr;
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

CIconGroup* CImageStoresSvc::FindIconGroup( UINT cmdId ) const
{
	// reverse iterate so the most recent store has priority (stack top, at the back)
	for ( std::vector<ui::IImageStore*>::const_reverse_iterator itStore = m_imageStores.rbegin(); itStore != m_imageStores.rend(); ++itStore )
		if ( CIconGroup* pFoundIconGroup = (*itStore)->FindIconGroup( cmdId ) )
			return pFoundIconGroup;

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

void CImageStoresSvc::QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const
{
	for ( std::vector<ui::IImageStore*>::const_iterator itStore = m_imageStores.begin(); itStore != m_imageStores.end(); ++itStore )
		(*itStore)->QueryToolbarDescriptors( rToolbarDescrs );
}

void CImageStoresSvc::QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const
{
	for ( std::vector<ui::IImageStore*>::const_iterator itStore = m_imageStores.begin(); itStore != m_imageStores.end(); ++itStore )
		(*itStore)->QueryToolbarsWithButton( rToolbarDescrs, cmdId );
}

void CImageStoresSvc::QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize /*= AnyIconSize*/ ) const
{
	for ( std::vector<ui::IImageStore*>::const_iterator itStore = m_imageStores.begin(); itStore != m_imageStores.end(); ++itStore )
		(*itStore)->QueryIconKeys( rIconKeys, iconStdSize );
}
