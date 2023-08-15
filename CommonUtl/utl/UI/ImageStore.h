#ifndef ImageStore_h
#define ImageStore_h
#pragma once

#include <unordered_map>
#include "utl/StdHashValue.h"
#include "Icon.h"


struct CThemeItem;
class CToolImageList;
namespace ui { struct CCmdAlias; }


class CImageStore : public ui::IImageStore, private utl::noncopyable
{
public:
	CImageStore( void );
	~CImageStore();

	void Clear( void );
public:
	// ui::IImageStore interface
	virtual CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon ) const;
	virtual const CIcon* RetrieveIcon( const CIconId& cmdId );
	virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor );

	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId );
	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps );
public:
	void RegisterToolbarImages( UINT toolBarId, COLORREF transpColor = color::Auto );
	void RegisterButtonImages( const CToolImageList& toolImageList );
	void RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, const CSize* pImageSize = nullptr );
	void RegisterIcon( UINT cmdId, CIcon* pIcon );		// takes ownership of pIcon
	void RegisterIcon( UINT cmdId, HICON hIcon ) { return RegisterIcon( cmdId, CIcon::NewIcon( hIcon ) ); }

	// aliases are registered in this store, as well as in afxCommandManager (for MFC control-bars)
	void RegisterAlias( UINT cmdId, UINT iconId );
	void RegisterAliases( const ui::CCmdAlias iconAliases[], size_t count );
private:
	CBitmap* RenderMenuBitmap( const CIcon& icon, bool checked ) const;
private:
	UINT FindAliasIconId( UINT cmdId ) const
	{
		std::unordered_map<UINT, UINT>::const_iterator itFound = m_cmdAliasMap.find( cmdId );
		if ( itFound != m_cmdAliasMap.end() )
			return itFound->second;					// found icon alias for the command
		return cmdId;
	}
private:
	std::unordered_map<UINT, UINT> m_cmdAliasMap;		// cmdId -> iconId: multiple commands sharing the same icon

	typedef std::pair<UINT, IconStdSize> TIconKey;		// <iconId, IconStdSize> - synonym with CIconId with hash value convenience
	typedef std::unordered_map<TIconKey, CIcon*, utl::CPairHasher> TIconMap;
	TIconMap m_iconMap;

	typedef std::pair<UINT, COLORREF> TBitmapKey;		// <iconId, transpColor>
	typedef std::unordered_map<TBitmapKey, CBitmap*, utl::CPairHasher> TBitmapMap;
	TBitmapMap m_bitmapMap;				// regular bitmaps look better than menu bitmaps because they retain the alpha channel

	std::unordered_map<UINT, TBitmapPair> m_menuBitmapMap;	// <iconId, <unchecked, checked> >

	std::auto_ptr<CThemeItem> m_pMenuItemBkTheme;
	std::auto_ptr<CThemeItem> m_pCheckedMenuItemBkTheme;
};


// singleton for global image lookup in all created image stores

class CImageStoresSvc : public ui::IImageStore, private utl::noncopyable
{
	CImageStoresSvc( void );

	static CImageStoresSvc* GetInstance( void );

	friend class CImageStore;

	void PushStore( ui::IImageStore* pImageStore );
	void PopStore( ui::IImageStore* pImageStore );
public:
	static ui::IImageStore* Instance( void ) { return GetInstance(); }

	// ui::IImageStore interface
	virtual CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon ) const;
	virtual const CIcon* RetrieveIcon( const CIconId& cmdId );
	virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor );

	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId );
	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps );
private:
	std::vector<ui::IImageStore*> m_imageStores;		// no ownership
};


#endif // ImageStore_h
