#ifndef FileBrowser_h
#define FileBrowser_h
#pragma once

#include "PathSortOrder.h"
#include "utl/Path.h"
#include "utl/PathItemBase.h"
#include <set>


class CFileItem;
class CPathIndex;
struct CFolderOptions;


class CFolderItem : public CPathItemBase
{
public:
	CFolderItem( const std::tstring& alias );
	CFolderItem( CFolderItem* pParentFolder, const fs::CPath& folderPath, const std::tstring& wildSpecs, const std::tstring& alias );
	CFolderItem( CFolderItem* pParentFolder, const fs::CPath& folderPath );			// nested folders
	virtual ~CFolderItem();

	void Clear( void );

	const std::vector< CFileItem* >& GetFileItems( void ) const { return m_files; }
	const std::vector< CFolderItem* >& GetSubFolders( void ) const { return m_subFolders; }

	bool HasAnyLeafs( void ) const { return m_deepLeafCount != 0; }

	CFileItem* FindFilePath( const fs::CPath& filePath ) const;

	CFolderItem* GetParentFolder( void ) const { return m_pParentFolder; }
	bool IsRootFolder( void ) const { return NULL == m_pParentFolder; }
	bool IsTopFolder( void ) const { return IsRootFolder() || m_pParentFolder->IsRootFolder(); }

	const std::tstring& GetAlias( void ) const { return m_alias; }
	std::tstring FormatAlias( void ) const;

	void SortFileItems( const CPathSortOrder& fileSortOrder );
	void SortSubFolders( void );

	// operations
	void SearchForFiles( RecursionDepth depth, CPathIndex* pPathIndex );
	bool AddFileItem( CPathIndex* pPathIndex, const fs::CPath& filePath, const std::tstring& fileLabel = std::tstring() );
	void AddSubFolderItem( CFolderItem* pSubFolder );
private:
	void AddLeafCount( size_t leafCount );
private:
	std::tstring m_wildSpecs;
	std::tstring m_folderDirName;
	std::tstring m_alias;

	// composite
	CFolderItem* m_pParentFolder;
	std::vector< CFileItem* > m_files;			// files in folder
	std::vector< CFolderItem* > m_subFolders;	// sub-folders in folder (if deep recurse enabled)
	size_t m_deepLeafCount;
};


class CFileItem : public CPathItemBase
{
public:
	CFileItem( const CFolderItem* pParentFolder, const fs::CPath& filePath, const std::tstring& label = std::tstring() );
	virtual ~CFileItem();

	const fs::CPathSortParts& GetSortParts( void ) const { return m_sortParts; }
	bool HasFilePath( const fs::CPath& rightFilePath ) const;

	std::tstring FormatLabel( const CFolderOptions* pOptions ) const;
private:
	const CFolderItem* m_pParentFolder;
private:
	const fs::CPathSortParts m_sortParts;

	std::tstring m_labelName;
	std::tstring m_labelExt;
};


class CPathIndex
{
public:
	CPathIndex( void ) {}

	bool Contains( const fs::CPath& filePath ) const { return m_allPaths.find( filePath ) != m_allPaths.end(); }
	bool RegisterUnique( const fs::CPath& filePath ) { return m_allPaths.insert( filePath ).second; }
private:
	std::set< fs::CPath > m_allPaths;		// unique files and folders
};


#include "utl/UI/RegistryOptions.h"


struct CFolderOptions : public CRegistryOptions
{
	CFolderOptions( const TCHAR* pSubSection = _T("") );		// pass "" for using the root section
	~CFolderOptions();

	void SetSubSection( const TCHAR* pSubSection );

	// base overrides
	virtual void LoadAll( void );

	DWORD GetFlags( void ) const;
	void SetFlags( DWORD flags );
private:
	PathField PathFieldFromCmd( UINT cmdId );
	UINT CmdFromPathField( PathField field );
public:
	// persistent options
	bool m_recurseFolders;
	bool m_cutDuplicates;

	bool m_displayFullPath;
	bool m_hideExt;
	bool m_rightJustifyExt;
	bool m_dirNamePrefix;

	bool m_noOptionsPopup;
	bool m_sortFolders;					// for root folders (all other sub-folders are sorted automatically)
	FolderLayout m_folderLayout;
	CPathSortOrder m_fileSortOrder;

	// generated overrides
protected:
	virtual void OnUpdateOption( CCmdUI* pCmdUI );
protected:
	afx_msg void OnToggle_SortBy( UINT cmdId );
	afx_msg void OnUpdate_SortBy( CCmdUI* pCmdUI );
	afx_msg void OnResetSortOrder( void );
	afx_msg void OnUpdateResetSortOrder( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#include "Application_fwd.h"


class CTargetMenu
{
public:
	CTargetMenu( void ) : m_initialCount( 0 ) {}

	void InitInplace( CMenu* pParentMenu );
	void InitAsSubMenu( const std::tstring& subMenuCaption );

	bool IsNull( void ) const { return NULL == m_menu.GetSafeHmenu() ; }
	bool IsEmpty( void ) const { return IsNull() || m_menu.GetMenuItemCount() == m_initialCount; }
	bool IsSubMenu( void ) const { return !m_subMenuCaption.empty(); }

	void Commit( CMenu* pParentMenu );

	CMenu* GetMenu( void ) { return &m_menu; }

	bool AppendItem( UINT itemId, const std::tstring& itemCaption ) { return DoAppendItem( &m_menu, MF_STRING, itemId, itemCaption.c_str() ); }
	bool AppendSeparator( void ) { return DoAppendItem( &m_menu, MF_SEPARATOR ); }

	static bool AppendSeparator( CMenu* pMenu );
	static bool AppendSubMenu( CMenu* pMenu, HMENU hSubMenu, const std::tstring& subMenuCaption ) { return DoAppendItem( pMenu, MF_POPUP | MF_STRING, (UINT_PTR)hSubMenu, subMenuCaption.c_str() ); }
	static bool AppendContextSubMenu( CMenu* pMenu, app::ContextPopup popupIndex );

	static bool DoAppendItem( CMenu* pMenu, UINT flags, UINT_PTR itemId = 0, const TCHAR* pItemText = NULL );
private:
	CMenu m_menu;
	UINT m_initialCount;
	std::tstring m_subMenuCaption;
};


class CScopedTargetMenu : public CTargetMenu
{
public:
	CScopedTargetMenu( CMenu* pParentMenu ) : CTargetMenu(), m_pParentMenu( pParentMenu ) { ASSERT_PTR( m_pParentMenu->GetSafeHmenu() ); }
	~CScopedTargetMenu() { Commit( m_pParentMenu ); }
private:
	using CTargetMenu::Commit;
private:
	CMenu* m_pParentMenu;
};


#include <hash_map>


class CFileMenuBuilder
{
public:
	CFileMenuBuilder( const CFolderOptions* pOptions );

	CMenu* GetPopupMenu( void ) { return &m_rootPopupMenu; }
	const CFileItem* FindFileItemWithId( UINT cmdId ) const;
	UINT MarkCurrFileItemId( const fs::CPath& currFilePath );

	bool BuildFolderItem( const CFolderItem* pFolderItem ) { return BuildFolderItem( &m_rootPopupMenu, pFolderItem ); }
private:
	bool BuildFolderItem( CMenu* pParentMenu, const CFolderItem* pFolderItem );
	void AppendFolderItem( CTargetMenu* pTargetMenu, const CFolderItem* pFolderItem );
	void AppendSubFolders( CTargetMenu* pTargetMenu, const std::vector< CFolderItem* >& subFolders );
	void AppendFileItems( CTargetMenu* pTargetMenu, const std::vector< CFileItem* >& fileItems );
	bool RegisterMenuUniqueItem( const CMenu* pPopupMenu, const CFileItem* pItem );

	bool UseSubMenu( const CFolderItem* pFolderItem ) const;
	UINT GetNextFileItemId( void ) { return m_fileItemId++; }
private:
	typedef stdext::hash_map< UINT, const CFileItem* > TMapIdToItem;

	typedef std::pair< HMENU, fs::CPath > TMenuPathKey;
	typedef stdext::hash_map< TMenuPathKey, const CFileItem* > TMapMenuPathToItem;
private:
	const CFolderOptions* m_pOptions;
	UINT m_fileItemId;							// self-encapsulated
	CMenu m_rootPopupMenu;
	TMapIdToItem m_idToItemMap;
	TMapMenuPathToItem m_menuPathToItemMap;		// to avoid duplicate folder-item or file-item paths in the same popup menu
};


class CFileBrowser : public CCmdTarget
{
public:
	CFileBrowser( void );
	~CFileBrowser();

	bool AddFolder( const fs::CPath& folderPathFilter, const std::tstring& folderAlias );
	bool AddFolderItems( const TCHAR* pFolderItems );
	bool AddRootFile( const fs::CPath& filePath, const std::tstring& label /*= std::tstring()*/ );

	const fs::CPath& GetCurrFilePath( void ) const { return m_currFilePath; }
	void SetCurrFilePath( const fs::CPath& currFilePath ) { m_currFilePath = currFilePath; }

	bool PickFile( CPoint screenPos );
private:
	struct CTrackInfo
	{
		CTrackInfo( const CFolderOptions* pOptions )
			: m_menuBuilder( pOptions ), m_keepTracking( false ) {}
	public:
		CFileMenuBuilder m_menuBuilder;
		bool m_keepTracking;
	};


	void SortItems( void );
	CMenu* BuildMenu( void );
	CFileMenuBuilder* GetMenuBuilder( void ) { return m_pTrackInfo.get() != NULL ? &m_pTrackInfo->m_menuBuilder : NULL; }
	static bool IsFileCmd( UINT cmdId );
public:
	CFolderOptions m_options;
private:
	std::auto_ptr< CFolderItem > m_pRootFolderItem;
	fs::CPath m_currFilePath;
	std::auto_ptr< CPathIndex > m_pPathIndex;

	std::auto_ptr< CTrackInfo > m_pTrackInfo;

	// generated overrides
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnCommand_FileItem( UINT cmdId );
	afx_msg void OnUpdate_FileItem( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace func
{
	struct ToFolderAlias
	{
		const std::tstring& operator()( const CFolderItem* pFolderItem ) const { return pFolderItem->GetAlias(); }
	};
}


namespace pred
{
	typedef CompareAdapterPtr< CompareNaturalPath, func::ToFolderAlias > TCompareFolderItem;

	struct CompareFileItem
	{
		CompareFileItem( const CPathSortOrder* pSortOrder ) : m_pSortOrder( pSortOrder ) { ASSERT( m_pSortOrder != NULL && !m_pSortOrder->IsEmpty() ); }

		CompareResult operator()( const CFileItem* pLeft, const CFileItem* pRight ) const;
	private:
		const CPathSortOrder* m_pSortOrder;
	};
}


#endif // FileBrowser_h
