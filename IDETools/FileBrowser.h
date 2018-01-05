
#ifndef FileBrowser_h
#define FileBrowser_h
#pragma once

#include <set>
#include "PublicEnums.h"
#include "PathInfo.h"
#include "MenuTrackingWindow.h"


struct CFolderOptions;


class CMetaFolder
{
public:
	CMetaFolder( CFolderOptions& rOptions, const CString& pathFilter, const CString& alias );
	~CMetaFolder();

	bool operator<( const CMetaFolder& cmp ) const { return lstrcmpi( m_alias, cmp.m_alias ) < 0; }

	struct CFile;

	// attributes
	bool IsValid( void ) const { return getFileCount( true ) != 0; }
	int getFileCount( bool deep = false ) const;
	CFile* getFile( unsigned int index ) const { ASSERT( index < m_files.size() ); return m_files[ index ]; }
	bool containsFilePath( const CString& fullFilePath, bool deep = false ) const { return findFilePath( fullFilePath, deep ) != NULL; }
	CFile* findFilePath( const CString& fullFilePath, bool deep ) const;
	bool containsFileWithId( UINT id, bool deep = false ) const { return findFileWithId( id, deep ) != NULL; }
	CFile* findFileWithId( UINT id, bool deep = true ) const;

	bool hasSubFolders( void ) const { return !m_subFolders.empty(); }
	int getSubFolderCount( void ) const { return (int)m_subFolders.size(); }
	CMetaFolder* getSubFolder( unsigned int index ) const { ASSERT( index < m_subFolders.size() ); return m_subFolders[ index ]; }

	bool isRootFolder( void ) const { return NULL == m_pParentFolder; }
	CMetaFolder* getParentFolder( void ) const { return m_pParentFolder; }
	void setParentFolder( CMetaFolder* pParentFolder ) { m_pParentFolder = pParentFolder; }

	CString getFolderAlias( const CString& prefix = m_defaultPrefix ) const;

	// operations
	bool addFile( const CString& filePath, const CString& fileLabel = CString() );
	bool searchForFiles( void );
	bool searchSubFolders( void );
	bool addFilesToMenu( CMenu& rootMenu, CMenu& menu, FolderLayout folderLayout = flRootFoldersExpanded ) const;
	void Clear( void );

	// helpers
	bool appendMenu( CMenu& destMenu, UINT flags, UINT itemId = 0, LPCTSTR pItemText = NULL ) const;
private:
	void splitFilters( void );
	bool addSubFoldersToMenu( CMenu& rootMenu, CMenu& menu, FolderLayout folderLayout ) const;
public:
	struct CFile
	{
		CFile( const CMetaFolder& rOwner, const CString& _filePath, const CString& _label );
		~CFile();

		bool operator==( const CFile& cmp ) const { return pred::Equal == Compare( cmp ); }
		bool operator<( const CFile& cmp ) const { return pred::Less == Compare( cmp ); }
		pred::CompareResult Compare( const CFile& cmp ) const;

		bool hasFileName( const PathInfo& filePathCmp ) const { return m_pathInfo.smartNameExtEQ( filePathCmp ); }

		CString getLabel( void ) const;
		CString getLabel( bool hideExt, bool rightJustifyExt, bool dirNamePrefix ) const;
		void setLabel( const CString& _label );
	public:
		const CMetaFolder& m_rOwner;
		CFolderOptions& m_rOptions;

		PathInfoEx m_pathInfo;
		CString m_label;
		CString m_labelName;
		CString m_labelExt;
		UINT m_menuId;
	};
public:
	CFolderOptions& m_rOptions;
	CString m_pathFilter;
	CString m_alias;
	CString m_folderPath;
	CString m_flatFilters;
	CString m_folderDirName;
	std::vector< CString > m_filters;

	static CString m_defaultPrefix;
private:
	CMetaFolder* m_pParentFolder;				// NULL if a root folder.
	std::vector< CFile* > m_files;				// files in folder.
	std::vector< CMetaFolder* > m_subFolders;	// sub-folders in folder (only if recursing enabled).
};


namespace pred
{
	struct LessMetaFolder
	{
		bool operator()( const CMetaFolder* pLeft, const CMetaFolder* pRight ) const
		{
			return *pLeft < *pRight;
		}
	};

	struct LessFile
	{
		bool operator()( const CMetaFolder::CFile* pLeft, const CMetaFolder::CFile* pRight ) const
		{
			return *pLeft < *pRight;
		}
	};
}


struct CFolderOptions : public IMenuCommandTarget
{
	CFolderOptions( LPCTSTR sectionPostfix = _T("") );
	~CFolderOptions();

	bool isDefaultSection( void ) const;
	const CString& setSection( LPCTSTR sectionPostfix );

	DWORD getFlags( void ) const;
	void setFlags( DWORD flags );

	bool loadProfile( void );
	bool saveProfile( void ) const;

	UINT getNextMenuId( void ) const { return m_menuFileItemId++; }
	UINT getMenuId( void ) const { return m_menuFileItemId; }

	CMenu& getOptionsPopup( void ) { return m_optionsPopup; }
	std::tstring loadOptionsPopup( void );

	// file and folder conditional adding
	bool contains( const CString& fileOrFolderPath ) const { return m_fileDirMap.find( fileOrFolderPath ) != m_fileDirMap.end(); }
	bool queryAddFileOrFolder( CString fileOrFolderPath, void* pFileOrFolder = NULL );

	void EnableMenuCommands( void );
	void updateSortOrderMenu( CMenu& rSortOrderPopup );
	void updateSortOrderMenu( void );

	// IMenuCommandTarget interface implementation
	virtual bool OnMenuCommand( UINT cmdId );

	static bool IsMenuStructureCommand( UINT cmdId );
	static bool IsFilepathSortingCommand( UINT cmdId );
private:
	CString m_section;
	mutable UINT m_menuFileItemId;

	std::set< CString, LessPathPred > m_fileDirMap;
	CMenu m_optionsPopup;
public:
	bool m_recurseFolders;
	bool m_cutDuplicates;
	bool m_hideExt;
	bool m_rightJustifyExt;
	bool m_dirNamePrefix;
	bool m_noOptionsPopup;
	bool m_sortFolders;

	CPathOrder m_fileSortOrder;
	FolderLayout m_folderLayout;

	CString m_selectedFileName;

	mutable CMetaFolder::CFile* m_pSelectedFileRef;
};


class CFileBrowser : public IMenuCommandTarget
{
public:
	CFileBrowser( void );
	~CFileBrowser();

	bool addFolder( const CString& folderPathFilter, const CString& folderAlias );
	bool addFolderStringArray( const CString& folderItemFlatArray );
	bool loadFolders( LPCTSTR section, LPCTSTR entry );
	bool addRootFile( const CString& filePath, const CString& label = CString() );
	bool overallExcludeFile( const CString& filePathFilter );
	bool excludeFileFromFolder( const CString& folderPath, const CString& fileFilter );

	bool pickFile( CPoint trackPos );

	int getFileTotalCount( void ) const;
	int getFolderCount( void ) const { return (int)m_folders.size(); }
	CMetaFolder* getFolder( unsigned int index ) const { ASSERT( index < m_folders.size() ); return m_folders[ index ]; }
	CMetaFolder::CFile* findFileWithId( UINT id ) const;

	// IMenuCommandTarget interface implementation
	virtual bool OnMenuCommand( UINT cmdId );
private:
	void buildMenu( CMenu& rOutMenu );
public:
	CFolderOptions m_options;
private:
	std::vector< CMetaFolder* > m_folders;
	CMetaFolder* m_pExtraFiles;
	bool m_extraFilesFirst;
};


#endif // FileBrowser_h
