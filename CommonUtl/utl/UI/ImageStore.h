#ifndef ImageStore_h
#define ImageStore_h
#pragma once

#include <hash_map>
#include "utl/StdHashValue.h"
#include "Icon.h"


struct CThemeItem;


class CImageStore
{
public:
	CImageStore( bool isShared = false );
	~CImageStore();

	static bool HasSharedStore( void ) { return m_pSharedStore != NULL; }
	static CImageStore* GetSharedStore( void ) { return m_pSharedStore; }
	static CImageStore* SharedStore( void ) { ASSERT_PTR( m_pSharedStore ); return m_pSharedStore; }

	void Clear( void );

	CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon ) const;

	const CIcon* RetrieveIcon( const CIconId& cmdId );
	const CIcon* RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize = HugeIcon_48 );
	static const CIcon* RetrieveSharedIcon( const CIconId& cmdId );

	CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor );
	CBitmap* RetrieveMenuBitmap( const CIconId& cmdId ) { return RetrieveBitmap( cmdId, ::GetSysColor( COLOR_MENU ) ); }

	std::pair< CBitmap*, CBitmap* > RetrieveMenuBitmaps( const CIconId& cmdId );
	std::pair< CBitmap*, CBitmap* > RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps );

	void RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, const CSize* pImageSize = NULL );
	void RegisterIcon( UINT cmdId, CIcon* pIcon );		// takes ownership of pIcon
	void RegisterIcon( UINT cmdId, HICON hIcon ) { return RegisterIcon( cmdId, CIcon::NewIcon( hIcon ) ); }

	int AddToImageList( CImageList& rImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize );

	struct CCmdAlias { UINT m_cmdId, m_iconId; };

	void RegisterAlias( UINT cmdId, UINT iconId );
	void RegisterAliases( const CCmdAlias iconAliases[], size_t count );
private:
	CBitmap* RenderMenuBitmap( const CIcon& icon, bool checked ) const;
private:
	UINT FindAliasIconId( UINT cmdId ) const
	{
		stdext::hash_map< UINT, UINT >::const_iterator itFound = m_cmdAliasMap.find( cmdId );
		if ( itFound != m_cmdAliasMap.end() )
			return itFound->second;					// found icon alias for the command
		return cmdId;
	}
private:
	static CImageStore* m_pSharedStore;

	stdext::hash_map< UINT, UINT > m_cmdAliasMap;		// cmdId -> iconId: multiple commands sharing the same icon

	typedef std::pair< UINT, IconStdSize > IconKey;		// <iconId, IconStdSize> - synonym with CIconId with hash value convenience
	stdext::hash_map< IconKey, CIcon* > m_iconMap;

	typedef std::pair< UINT, COLORREF > BitmapKey;		// <iconId, transpColor>
	stdext::hash_map< BitmapKey, CBitmap* > m_bitmapMap;	// regular bitmaps look better than menu bitmaps because they retain the alpha channel

	stdext::hash_map< UINT, std::pair< CBitmap*, CBitmap* > > m_menuBitmapMap;		// <iconId, <unchecked, checked> >

	std::auto_ptr<CThemeItem> m_pMenuItemBkTheme;
	std::auto_ptr<CThemeItem> m_pCheckedMenuItemBkTheme;
};


#endif // ImageStore_h
