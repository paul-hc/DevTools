
#include "stdafx.h"
#include "ImageStore.h"
#include "Imaging.h"
#include "ContainerUtilities.h"
#include "Dialog_fwd.h"
#include "ThemeItem.h"

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

CImageStore* CImageStore::m_pSharedStore = NULL;

CImageStore::CImageStore( bool isShared /*= false*/ )
	: m_pMenuItemBkTheme( new CThemeItem( _T("MENU"), MENU_POPUPBACKGROUND, 0 ) )							// item background (opaque)
	, m_pCheckedMenuItemBkTheme( new CThemeItem( _T("MENU"), MENU_POPUPCHECKBACKGROUND, MCB_BITMAP ) )		// checked button background (semi-transparent)
{
	if ( isShared )
	{
		ASSERT_NULL( m_pSharedStore );		// create shared store only once
		m_pSharedStore = this;
	}
}

CImageStore::~CImageStore()
{
	Clear();

	if ( m_pSharedStore == this )
		m_pSharedStore = NULL;
}

void CImageStore::Clear( void )
{
	utl::ClearOwningAssocContainerValues( m_iconMap );
	utl::ClearOwningAssocContainerValues( m_bitmapMap );
	utl::ClearOwningContainer( m_menuBitmapMap, func::DeleteMenuBitmaps() );
}

CIcon* CImageStore::FindIcon( UINT cmdId, IconStdSize iconStdSize /*= SmallIcon*/ ) const
{
	const IconKey key( FindAliasIconId( cmdId ), iconStdSize );
	stdext::hash_map< IconKey, CIcon* >::const_iterator itFound = m_iconMap.find( key );
	if ( itFound == m_iconMap.end() )
		return NULL;
	return itFound->second;
}

void CImageStore::RegisterAlias( UINT cmdId, UINT iconId )
{
	ASSERT( cmdId != 0 && iconId != 0 );
	m_cmdAliasMap[ cmdId ] = iconId;
}

void CImageStore::RegisterAliases( const CCmdAlias iconAliases[], size_t count )
{
	for ( size_t i = 0; i != count; ++i )
		RegisterAlias( iconAliases[ i ].m_cmdId, iconAliases[ i ].m_iconId );
}

const CIcon* CImageStore::RetrieveIcon( const CIconId& cmdId )
{
	if ( CIcon* pFoundIcon = FindIcon( cmdId.m_id, cmdId.m_stdSize ) )
		return pFoundIcon;

	const IconKey iconKey( FindAliasIconId( cmdId.m_id ), cmdId.m_stdSize );		// try loading iconId always (not the command alias)
	if ( CIcon* pIcon = CIcon::NewExactIcon( CIconId( iconKey.first, cmdId.m_stdSize ) ) )
	{
		m_iconMap[ iconKey ] = pIcon;
		return pIcon;
	}

	return NULL;
}

const CIcon* CImageStore::RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize /*= HugeIcon_48*/ )
{
	for ( ; maxIconStdSize >= DefaultSize; --(int&)maxIconStdSize )
		if ( const CIcon* pIcon = RetrieveIcon( CIconId( cmdId, maxIconStdSize ) ) )
			return pIcon;
	return NULL;
}

const CIcon* CImageStore::RetrieveSharedIcon( const CIconId& cmdId )
{
	if ( m_pSharedStore != NULL )
		return m_pSharedStore->RetrieveIcon( cmdId );
	return NULL;
}

CBitmap* CImageStore::RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor )
{
	const BitmapKey key( cmdId.m_id, transpColor );
	stdext::hash_map< BitmapKey, CBitmap* >::const_iterator itFound = m_bitmapMap.find( key );
	if ( itFound != m_bitmapMap.end() )
		return itFound->second;

	if ( const CIcon* pIcon = RetrieveIcon( cmdId ) )
	{
		ASSERT( pIcon->IsValid() );

		CBitmap* pBitmap = new CBitmap;
		pIcon->MakeBitmap( *pBitmap, transpColor );
		m_bitmapMap[ key ] = pBitmap;
		return pBitmap;
	}
	return NULL;
}

std::pair< CBitmap*, CBitmap* > CImageStore::RetrieveMenuBitmaps( const CIconId& cmdId )
{
	stdext::hash_map< UINT, std::pair< CBitmap*, CBitmap* > >::const_iterator itFound = m_menuBitmapMap.find( cmdId.m_id );
	if ( itFound != m_menuBitmapMap.end() )
		return itFound->second;

	if ( const CIcon* pIcon = RetrieveIcon( cmdId ) )
	{
		ASSERT( pIcon->IsValid() );

		std::pair< CBitmap*, CBitmap* >& rBitmapPair = m_menuBitmapMap[ cmdId.m_id ];
		rBitmapPair.first = RenderMenuBitmap( *pIcon, false );		// unchecked state
		rBitmapPair.second = RenderMenuBitmap( *pIcon, true );		// checked state
		return rBitmapPair;
	}
	return std::pair< CBitmap*, CBitmap* >( NULL, NULL );
}

std::pair< CBitmap*, CBitmap* > CImageStore::RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps )
{
	if ( useCheckedBitmaps )
		return RetrieveMenuBitmaps( cmdId );		// use rendered DDBs
	else
	{
		// straight bitmap looks better because the DIB section retains the alpha channel;
		// no image for checked state (will use a check mark)
		return std::pair< CBitmap*, CBitmap* >( RetrieveBitmap( cmdId, GetSysColor( COLOR_MENU ) ), NULL );
	}
}

CBitmap* CImageStore::RenderMenuBitmap( const CIcon& icon, bool checked ) const
{
	enum { Edge = 2 };

	CSize iconSize = icon.GetSize();
	CSize bitmapSize = iconSize + CSize( Edge * 2, Edge * 2 );

	std::auto_ptr< CBitmap > pMenuBitmap( new CBitmap );
	CWindowDC screenDC( NULL );
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

	return NULL;
}

void CImageStore::RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, const CSize* pImageSize /*= NULL*/ )
{
	ASSERT_PTR( imageList.GetSafeHandle() );
	ASSERT( imageList.GetImageCount() <= (int)buttonCount );		// give or take the separators

	const bool hasAlpha = !gdi::HasMask( imageList, 0 );			// could also use ui::HasAlphaTransparency() - assume all images are the same
	CSize imageSize = pImageSize != NULL ? *pImageSize : gdi::GetImageSize( imageList );
	IconKey iconKey;

	switch ( imageSize.cx )
	{
		default: ASSERT( false );
		case 16: iconKey.second = SmallIcon; break;
		case 24: iconKey.second = MediumIcon; break;
		case 32: iconKey.second = LargeIcon; break;
		case 48: iconKey.second = HugeIcon_48; break;
	};

	for ( UINT i = 0, imagePos = 0; i != buttonCount; ++i )
		if ( buttonIds[ i ] != ID_SEPARATOR )			// skip separators
		{
			iconKey.first = buttonIds[ i ];

			stdext::hash_map< IconKey, CIcon* >::const_iterator itFound = m_iconMap.find( iconKey );
			if ( itFound == m_iconMap.end() )
			{
				HICON hIcon = const_cast< CImageList& >( imageList ).ExtractIcon( imagePos );
				ASSERT_PTR( hIcon );
				CIcon* pIcon = new CIcon( hIcon, imageSize );
				pIcon->SetHasAlpha( hasAlpha );

				m_iconMap[ iconKey ] = pIcon;
			}
			++imagePos;
		}
}

int CImageStore::AddToImageList( CImageList& rImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize )
{
	ASSERT( buttonIds != NULL && buttonCount != 0 );

	if ( NULL == rImageList.GetSafeHandle() )
		VERIFY( rImageList.Create( imageSize.cx, imageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 5 ) );

	IconStdSize iconStdSize = CIconId::FindStdSize( imageSize );
	int imageCount = 0;

	for ( size_t i = 0; i != buttonCount; ++i )
		if ( buttonIds[ i ] != 0 )			// skip separators
			if ( const CIcon* pIcon = RetrieveIcon( CIconId( buttonIds[ i ], iconStdSize ) ) )
			{
				rImageList.Add( pIcon->GetHandle() );
				++imageCount;
			}

	return imageCount;
}
