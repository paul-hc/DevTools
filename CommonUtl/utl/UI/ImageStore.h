#ifndef ImageStore_h
#define ImageStore_h
#pragma once

#include <unordered_map>
#include "utl/StdHashValue.h"		// for utl::CPairHasher
#include "ImageStore_fwd.h"
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
	virtual CIconGroup* FindIconGroup( UINT cmdId ) const;
	virtual const CIcon* RetrieveIcon( const CIconId& cmdId );
	virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor );

	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId );
	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps );

	virtual void QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const;
	virtual void QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const;
	virtual void QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize = AnyIconSize ) const;
public:
	void RegisterToolbarImages( UINT toolbarId, COLORREF transpColor = color::Auto );
	void RegisterButtonImages( const CToolImageList& toolImageList );
	void RegisterButtonImages( const CImageList& imageList, const UINT buttonIds[], size_t buttonCount, bool hasAlpha, const CSize* pImageSize = nullptr );
	void RegisterIcon( UINT cmdId, CIcon* pIcon );			// takes ownership of pIcon
	void RegisterIcon( UINT cmdId, HICON hIcon ) { return RegisterIcon( cmdId, CIcon::LoadNewIcon( hIcon ) ); }
	void RegisterLoadIcon( const CIconId& iconId );			// load icon from resources
	CIconGroup* RegisterLoadIconGroup( UINT iconId );		// load all frames (formats) of an icon from resources - a frame is a group icon directory entry with BPP and size.

	// aliases are registered in this store, as well as in afxCommandManager (for MFC control-bars)
	void RegisterAlias( UINT cmdId, UINT iconId );
	void RegisterAliases( const ui::CCmdAlias iconAliases[], size_t count );
private:
	UINT FindAliasIconId( UINT cmdId ) const;
	CIconGroup*& MapIconGroup( CIconGroup* pIconGroup );
	bool MapIcon( const ui::CIconKey& iconKey, CIcon* pIcon );

	CBitmap* RenderMenuBitmap( const CIcon& icon, bool checked ) const;
private:
	std::unordered_map<UINT, UINT> m_cmdAliasMap;			// cmdId -> iconId: multiple commands sharing the same icon
	std::vector<ui::CToolbarDescr*> m_toolbarDescriptors;	// buttons of the loaded toolbars (and corresponding icons)

	typedef std::unordered_map<UINT, CIconGroup*> TIconGroupMap;	// <iconResId, iconGroup>
	TIconGroupMap m_iconFramesMap;
	std::vector<ui::CIconKey> m_iconKeys;					// in registering order

	typedef std::pair<UINT, COLORREF> TBitmapKey;			// <iconId, transpColor>
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
	virtual CIconGroup* FindIconGroup( UINT cmdId ) const;
	virtual const CIcon* RetrieveIcon( const CIconId& cmdId );
	virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor );

	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId );
	virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps );

	virtual void QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const;
	virtual void QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const;
	virtual void QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize = AnyIconSize ) const;
private:
	std::vector<ui::IImageStore*> m_imageStores;		// no ownership
};


#endif // ImageStore_h
