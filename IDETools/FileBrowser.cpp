
#include "stdafx.h"
#include "FileBrowser.h"
#include "ModuleSession.h"
#include "IdeUtilities.h"
#include "StringUtilitiesEx.h"
#include "Application.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/ContainerUtilities.h"
#include "utl/MenuUtilities.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	bool SmartAppendMenu( int menuVertSplitCount, CMenu& destMenu, UINT flags, UINT itemId = 0, LPCTSTR pItemText = NULL )
	{
		ASSERT_PTR( destMenu.GetSafeHmenu() );

		int itemCount = destMenu.GetMenuItemCount();

		if ( HasFlag( flags, MF_SEPARATOR ) )
			if ( 0 == itemCount || 0 == destMenu.GetMenuItemID( itemCount - 1 ) )
				return false;

		if ( itemCount != 0 && 0 == ( itemCount % menuVertSplitCount ) )
			flags |= MF_MENUBARBREAK | MF_MENUBREAK;

		return destMenu.AppendMenu( flags, itemId, pItemText ) != FALSE;
	}
}


// CFileBrowser implementation

CFileBrowser::CFileBrowser( void )
	: m_options()
	, m_pExtraFiles( new CMetaFolder( m_options, CString(), _T("[Extra Root Files]") ) )
	, m_extraFilesFirst( false )
	, m_keepTracking( false )
{
}

CFileBrowser::~CFileBrowser()
{
	utl::ClearOwningContainer( m_folders );
	delete m_pExtraFiles;
}

int CFileBrowser::getFileTotalCount( void ) const
{
	unsigned int fileCount = 0;

	for ( unsigned int i = 0; i != m_folders.size(); ++i )
		fileCount += m_folders[ i ]->getFileCount( true );

	return fileCount;
}

bool CFileBrowser::addFolder( const CString& folderPathFilter, const CString& folderAlias )
{
	std::auto_ptr< CMetaFolder > pNewFolder( new CMetaFolder( m_options, folderPathFilter, folderAlias ) );

	if ( !m_options.queryAddFileOrFolder( folderPathFilter, pNewFolder.get() ) )
		return false;

	bool success = pNewFolder->searchForFiles();

	std::vector< CMetaFolder* >::iterator itInsert = m_folders.end();

	if ( m_options.m_sortFolders )
		itInsert = std::upper_bound( m_folders.begin(), m_folders.end(), pNewFolder.get(), pred::LessMetaFolder() );

	m_folders.insert( itInsert, pNewFolder.release() );

	return success;
}

bool CFileBrowser::addFolderStringArray( const CString& folderItemFlatArray )
{
	// folderItemFlatArray: folder items separated by EDIT_SEP=";" token (at end no token);
	// Each folder item contains a folder path filter, and optional, a folder alias separated
	// by PROF_SEP="|" separator.
	// example:
	//	"D:\Development\Tools\Slider\LogicalFilesBackup.h,LogicalFilesBackup.hxx,LogicalFilesBackup.cxx|ASSOCIATIONS;D:\Development\Tools\Slider\slider Desc\LogicalFilesBackup_Factory.*,LogicalFilesBackup_Tags.*|FACTORY"

	std::vector< CString > dualArray;

	str::split( dualArray, folderItemFlatArray, _T(";") );
	for ( unsigned int i = 0; i != dualArray.size(); ++i )
	{
		CString folderItem = dualArray[ i ];
		int sepPos = folderItem.Find( PROF_SEP );

		ASSERT( !folderItem.IsEmpty() );
		if ( sepPos != -1 )
			addFolder( folderItem.Left( sepPos ), folderItem.Mid( sepPos + 1 ) );	// alias is specified
		else
			addFolder( folderItem, _T("") );		// no alias
	}

	return !dualArray.empty();
}

bool CFileBrowser::loadFolders( LPCTSTR section, LPCTSTR entry )
{
	CString folderItemFlatArray = AfxGetApp()->GetProfileString( section, entry, _T("") );

	if ( folderItemFlatArray.IsEmpty() )
		return false;

	return addFolderStringArray( folderItemFlatArray );
}

bool CFileBrowser::addRootFile( const CString& filePath, const CString& label /*= CString()*/ )
{
	m_extraFilesFirst = m_folders.empty();
	return m_pExtraFiles->addFile( filePath, label );
}

bool CFileBrowser::overallExcludeFile( const CString& filePathFilter )
{
	filePathFilter;
	return false;
}

bool CFileBrowser::excludeFileFromFolder( const CString& folderPath, const CString& fileFilter )
{
	UNUSED_ALWAYS( folderPath );
	UNUSED_ALWAYS( fileFilter );

	return false;
}

void CFileBrowser::BuildMenu( CMenu& rOutMenu )
{
	rOutMenu.CreatePopupMenu();

	if ( m_extraFilesFirst && m_pExtraFiles->IsValid() )
	{
		m_pExtraFiles->addFilesToMenu( rOutMenu, rOutMenu );
		m_pExtraFiles->appendMenu( rOutMenu, MF_SEPARATOR );
	}

	for ( int folderIndex = 0, folderCount = getFolderCount(); folderIndex < folderCount; ++folderIndex )
	{
		const CMetaFolder& rootFolder = *getFolder( folderIndex );

		if ( rootFolder.addFilesToMenu( rOutMenu, rOutMenu, m_options.m_folderLayout ) )
			if ( m_options.m_folderLayout == flAllFoldersExpanded )
				rootFolder.appendMenu( rOutMenu, MF_SEPARATOR );
	}

	if ( !m_extraFilesFirst && m_pExtraFiles->IsValid() )
	{
		m_pExtraFiles->appendMenu( rOutMenu, MF_SEPARATOR );
		m_pExtraFiles->addFilesToMenu( rOutMenu, rOutMenu );
	}

	if ( !m_options.m_noOptionsPopup )
	{	// append options popup
		std::tstring optionsPopupText = m_options.loadOptionsPopup();

		hlp::SmartAppendMenu( app::GetModuleSession().m_menuVertSplitCount, rOutMenu, MF_SEPARATOR );
		rOutMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR)(HMENU)m_options.getOptionsPopup(), optionsPopupText.c_str() );
	}
}

CMetaFolder::CFile* CFileBrowser::findFileWithId( UINT id ) const
{
	const CMetaFolder::CFile* pFoundFile = NULL;

	for ( int i = 0; i < getFolderCount() && NULL == pFoundFile; ++i )
		pFoundFile = getFolder( i )->findFileWithId( id, true );

	if ( NULL == pFoundFile )
		pFoundFile = m_pExtraFiles->findFileWithId( id, true );

	return (CMetaFolder::CFile*)pFoundFile;
}

bool CFileBrowser::PickFile( CPoint screenPos )
{
	if ( -1 == screenPos.x && -1 == screenPos.y )
		::GetCursorPos( &screenPos );

	CWnd* pIdeWindow = ide::GetRootWindow();
	UINT cmdId = 0;

	do
	{
		m_keepTracking = false;

		CMenu popup;

		BuildMenu( popup );
		m_options.EnableMenuCommands();

		CMenuTrackingWindow* pTrackingWindow = new CMenuTrackingWindow( this, popup, MTM_CommandRepeat );

		if ( m_options.m_pSelectedFileRef != NULL )
			pTrackingWindow->SetHilightId( m_options.m_pSelectedFileRef->m_menuId );

		pTrackingWindow->Create( pIdeWindow );

		// Sometimes this fails to track the menu, due to some _AFX_THREAD_STATE related global state.
		// The workaround is to set pTrackingWindow as foreground window; or alternatively use CMenu::TrackPopupMenu() method, instead of TrackPopupMenuEx().
		pTrackingWindow->SetForegroundWindow();

		cmdId = ui::TrackPopupMenu( popup, pTrackingWindow, screenPos, TPM_NONOTIFY | TPM_RETURNCMD );		// TPM_RIGHTBUTTON creates problems?!
		// was: cmdId = popup.TrackPopupMenu( TPM_NONOTIFY | TPM_RETURNCMD, trackPos.x, trackPos.y, pTrackingWindow );

		if ( cmdId != 0 )
			if ( OnMenuCommand( cmdId ) )
				if ( CFolderOptions::IsMenuStructureCommand( cmdId ) )
					m_keepTracking = true;	// rebuild and track the menu

		pTrackingWindow->DestroyWindow();	// auto-delete

		pIdeWindow->SetForegroundWindow();

		DEBUG_LOG( _T("MenuId=%d, Selected File: %s"), cmdId, m_options.GetSelectedFileName().c_str() );

	} while ( m_keepTracking );

	return cmdId >= CM_FILEBROWSER_FILEITEM && !m_options.GetSelectedFileName().empty();
}

bool CFileBrowser::OnMenuCommand( UINT cmdId )
{
	if ( m_options.OnMenuCommand( cmdId ) )
		return true;

	if ( cmdId >= CM_FILEBROWSER_FILEITEM )
	{
		if ( CMetaFolder::CFile* pPickedFile = findFileWithId( cmdId ) )
		{
			if ( ui::IsKeyPressed( VK_CONTROL ) )
			{	// final command with CTRL pressed -> sent selected file path to the clipboard
				if ( CClipboard::CopyText( (LPCTSTR)pPickedFile->m_pathInfo.GetFullPath() ) )
					m_options.SetSelectedFileName( std::tstring() );			// reset the picked file path member in order to avoid the final execution
			}
			else
				m_options.SetSelectedFileName( pPickedFile->m_pathInfo.Get().GetString() );	// setup the picked file path
		}
		return true;
	}

	return false;
}


// CMetaFolder implementation

CString CMetaFolder::m_defaultPrefix( _T("+ ") );

CMetaFolder::CMetaFolder( CFolderOptions& rOptions, const CString& pathFilter, const CString& alias )
	: m_rOptions( rOptions )
	, m_pathFilter( pathFilter )
	, m_alias( !alias.IsEmpty() ? alias : m_pathFilter )
	, m_pParentFolder( NULL )
{
	if ( !m_pathFilter.IsEmpty() )
		splitFilters();
}

CMetaFolder::~CMetaFolder()
{
	Clear();
}

void CMetaFolder::Clear( void )
{
	utl::ClearOwningContainer( m_files );
	utl::ClearOwningContainer( m_subFolders );
}

void CMetaFolder::splitFilters( void )
{
	int slashPos = m_pathFilter.Find( _T(',') ), slashPos2;

	m_filters.clear();

	// reverse searching for last slash starting from first comma separator for filters (if any), otherwise from the end
	// in order to allow relative path filter specifiers
	slashPos = PathInfo::revFindSlashPos( m_pathFilter, slashPos );

	if ( slashPos != -1 )
	{
		m_folderDirName = m_pathFilter.Left( slashPos );
		if ( !m_folderDirName.IsEmpty() )
			if ( ( slashPos2 = PathInfo::revFindSlashPos( m_folderDirName ) ) != -1 )
				m_folderDirName = m_folderDirName.Mid( slashPos2 + 1 );

		++slashPos;

		m_folderPath = m_pathFilter.Left( slashPos );	// assign folder path including the trailing slash.
		m_flatFilters = m_pathFilter.Mid( slashPos );	// assign the filter including the leading slash.
		ASSERT( !m_flatFilters.IsEmpty() );

		str::split( m_filters, m_flatFilters, _T(",") );
	}
}

int CMetaFolder::getFileCount( bool deep /*= false*/ ) const
{
	int fileCount = (int)m_files.size();

	if ( deep )
		for ( unsigned int i = 0; i != m_subFolders.size(); ++i )
			fileCount += m_subFolders[ i ]->getFileCount( deep );

	return fileCount;
}

bool CMetaFolder::searchForFiles( void )
{
	CFileFind searchEngine;

	// first: add files in current folder
	for ( unsigned int filterIndex = 0; filterIndex != m_filters.size(); ++filterIndex )
	{
		CString currFilter( m_folderPath + m_filters[ filterIndex ] ), foundFile;

		for ( BOOL hit = searchEngine.FindFile( currFilter ); hit; )
		{
			hit = searchEngine.FindNextFile();
			foundFile = searchEngine.GetFilePath();
			if ( !searchEngine.IsDirectory() && !searchEngine.IsDots() )
				addFile( foundFile, m_rOptions.m_hideExt ? searchEngine.GetFileTitle()
													: searchEngine.GetFileName() );
		}
	}

	// second: add sub-folders in current folder, if desired and any
	if ( m_rOptions.m_recurseFolders )
		searchSubFolders();

	return IsValid();
}

bool CMetaFolder::searchSubFolders( void )
{
	CString currFilter( m_folderPath + _T("*.*") );
	FileFindEx searchEngine;

	for ( BOOL hit = searchEngine.findDir( currFilter ); hit; )
	{
		hit = searchEngine.FindNextFile();
		if ( searchEngine.IsDirectory() && !searchEngine.IsDots() )
		{
			CString subFolderFilter = searchEngine.GetFilePath() + _T('\\') + m_flatFilters;
			std::auto_ptr< CMetaFolder > pNewFolder( new CMetaFolder( m_rOptions, subFolderFilter, searchEngine.GetFileName() ) );

			pNewFolder->setParentFolder( this );
			if ( !m_rOptions.queryAddFileOrFolder( subFolderFilter, pNewFolder.get() ) )
				return false;

			pNewFolder->searchForFiles();

			std::vector< CMetaFolder* >::iterator itInsert = m_subFolders.end();

			if ( m_rOptions.m_sortFolders )
				itInsert = std::upper_bound( m_subFolders.begin(), m_subFolders.end(), pNewFolder.get(), pred::LessMetaFolder() );

			m_subFolders.insert( itInsert, pNewFolder.release() );
		}
	}

	return hasSubFolders();
}

bool CMetaFolder::addFile( const CString& filePath, const CString& fileLabel /*= CString()*/ )
{
	if ( containsFilePath( filePath, false ) )
		return false;		// avoid file duplicates in one folder !

	std::auto_ptr< CFile > pNewFile( new CFile( *this, filePath, fileLabel ) );

	if ( !m_rOptions.queryAddFileOrFolder( filePath, pNewFile.get() ) )
		return false;

	std::vector< CFile* >::iterator itInsert = m_files.end();

	if ( !m_rOptions.m_fileSortOrder.IsEmpty() )
		itInsert = std::upper_bound( m_files.begin(), m_files.end(), pNewFile.get(), pred::LessFile() );

	m_files.insert( itInsert, pNewFile.release() );
	return true;
}

CString CMetaFolder::getFolderAlias( const CString& prefix /*= m_defaultPrefix*/ ) const
{
	return prefix + m_alias;
}

bool CMetaFolder::addFilesToMenu( CMenu& rootMenu, CMenu& menu, FolderLayout folderLayout /*= flRootFoldersExpanded*/ ) const
{
	// avoid adding deep-empty / empty folders, depending on 'folderLayout'
	if ( !IsValid() )
		return false;

	CMenu targetMenu;
	bool newPopup = true;

	if ( folderLayout == flRootFoldersExpanded )
		newPopup = !isRootFolder();
	else if ( folderLayout == flAllFoldersExpanded )
		newPopup = false;

	if ( !newPopup )
		targetMenu.Attach( (HMENU)menu );
	else
	{
		if ( folderLayout != flFoldersAsRootPopups )
		{
			targetMenu.CreatePopupMenu();
			appendMenu( menu, MF_POPUP | MF_STRING, (UINT_PTR)(HMENU)targetMenu, getFolderAlias() );
		}
		else if ( getFileCount( false ) > 0 )
		{	// flFoldersAsRootPopups: add popup to root only if has it's own files
			targetMenu.CreatePopupMenu();
			appendMenu( rootMenu, MF_POPUP | MF_STRING, (UINT_PTR)(HMENU)targetMenu, getFolderAlias() );
		}
	}

	bool subFoldersComesFirst = folderLayout == flFoldersAsPopups || folderLayout == flRootFoldersExpanded;

	if ( subFoldersComesFirst )
		addSubFoldersToMenu( rootMenu, targetMenu, folderLayout );	// recurse into sub-folders (if any)

	PathInfo currFilePath( m_rOptions.GetSelectedFileName().c_str() );

	// add files
	for ( int index = 0, fileCount = getFileCount( false ); index < fileCount; ++index )
	{
		CMetaFolder::CFile& file = *getFile( index );

		file.m_menuId = m_rOptions.getNextMenuId();
		appendMenu( targetMenu, MF_STRING, file.m_menuId, file.getLabel() );
		if ( !m_rOptions.GetSelectedFileName().empty() && file.hasFileName( currFilePath ) )
		{
			targetMenu.CheckMenuRadioItem( file.m_menuId, file.m_menuId, file.m_menuId, MF_BYCOMMAND );
			if ( m_rOptions.m_pSelectedFileRef == NULL )
			{
				targetMenu.SetDefaultItem( file.m_menuId );
				m_rOptions.m_pSelectedFileRef = &file;
			}
		}
	}

	if ( !subFoldersComesFirst )
		addSubFoldersToMenu( rootMenu, targetMenu, folderLayout );	// recurse into sub-folders (if any)

	targetMenu.Detach();
	return true;
}

bool CMetaFolder::addSubFoldersToMenu( CMenu& rootMenu, CMenu& menu, FolderLayout folderLayout ) const
{
	// recurse into sub-folders an dreturn true if any:
	int addedCount = 0;
	for ( int index = 0; index < getSubFolderCount(); ++index )
		if ( getSubFolder( index )->addFilesToMenu( rootMenu, menu, folderLayout ) )
			++addedCount;

	return addedCount != 0;
}

bool CMetaFolder::appendMenu( CMenu& destMenu, UINT flags, UINT itemId /*= 0*/, LPCTSTR pItemText /*= NULL*/ ) const
{
	return hlp::SmartAppendMenu( app::GetModuleSession().m_menuVertSplitCount, destMenu, flags, itemId, pItemText );
}

CMetaFolder::CFile* CMetaFolder::findFilePath( const CString& fullFilePath, bool deep ) const
{
	CFile* pFoundFile = NULL;

	for ( int i = 0; pFoundFile == NULL && i < getFileCount( false ); ++i )
		if ( path::EquivalentPtr( getFile( i )->m_pathInfo.Get(), fullFilePath ) )
			pFoundFile = getFile( i );
	if ( deep )
		for ( int i = 0; pFoundFile == NULL && i < getSubFolderCount(); ++i )
			pFoundFile = getSubFolder( i )->findFilePath( fullFilePath, deep );

	return pFoundFile;
}

CMetaFolder::CFile* CMetaFolder::findFileWithId( UINT id, bool deep /*= true*/ ) const
{
	CFile* pFoundFile = NULL;

	for ( int i = 0; pFoundFile == NULL && i < getFileCount( false ); ++i )
		if ( getFile( i )->m_menuId == id )
			pFoundFile = getFile( i );

	if ( deep )
		for ( int i = 0; pFoundFile == NULL && i < getSubFolderCount(); ++i )
			pFoundFile = getSubFolder( i )->findFileWithId( id, deep );

	return pFoundFile;
}


// CMetaFolder::CFile implementation

CMetaFolder::CFile::CFile( const CMetaFolder& rOwner, const CString& _filePath, const CString& _label )
	: m_rOwner( rOwner )
	, m_rOptions( m_rOwner.m_rOptions )
	, m_pathInfo( _filePath )
	, m_menuId( 0 )
{
	setLabel( _label );
}

CMetaFolder::CFile::~CFile()
{
}

pred::CompareResult CMetaFolder::CFile::Compare( const CFile& cmp ) const
{
	return m_pathInfo.Compare( cmp.m_pathInfo, m_rOptions.m_fileSortOrder.m_fields, m_rOwner.m_folderDirName );
}

void CMetaFolder::CFile::setLabel( const CString& _label )
{
	m_label.Empty();
	m_labelName.Empty();
	m_labelExt.Empty();
	if ( !_label.IsEmpty() )
		m_label = _label;
	else
		// By default the label field is filename+ext from 'm_pathInfo':
		m_label = m_pathInfo.getNameExt();

	if ( !m_label.IsEmpty() )
	{
		int extPos = m_label.ReverseFind( _T('.') );

		if ( extPos != -1 )
		{
			m_labelName = m_label.Left( extPos );
			m_labelExt = m_label.Mid( extPos + 1 );
		}
		else
			m_labelName = m_label;
	}
}

CString CMetaFolder::CFile::getLabel( bool hideExt, bool rightJustifyExt, bool dirNamePrefix ) const
{
	CString retLabel;

	if ( dirNamePrefix )
		retLabel = m_pathInfo.dirName + _T('\\');

	if ( hideExt || m_labelExt.IsEmpty() )
		retLabel += m_labelName;
	else if ( rightJustifyExt )
		retLabel += m_labelName + _T('\t') + m_labelExt;
	else
		retLabel += m_label;
	return retLabel;
}

CString CMetaFolder::CFile::getLabel( void ) const
{
	return getLabel( m_rOptions.m_hideExt, m_rOptions.m_rightJustifyExt, m_rOptions.m_dirNamePrefix );
}


// CFolderOptions implementation

namespace reg
{
	static const TCHAR section_rootOptions[] = _T("FolderOptions");
}

CFolderOptions::CFolderOptions( LPCTSTR pSubSection /*= _T("")*/ )
	: m_menuFileItemId( CM_FILEBROWSER_FILEITEM )
	, m_recurseFolders( false )
	, m_cutDuplicates( false )
	, m_hideExt( false )
	, m_rightJustifyExt( false )
	, m_dirNamePrefix( true )
	, m_noOptionsPopup( false )
	, m_sortFolders( true )
	, m_folderLayout( flFoldersAsPopups )
	, m_pSelectedFileRef( NULL )
{
	SetSubSection( pSubSection );
	LoadAll();
}

CFolderOptions::~CFolderOptions()
{
	if ( !m_noOptionsPopup )
		m_optionsPopup.Detach();
}

void CFolderOptions::SetSubSection( const TCHAR* pSubSection )
{
	if ( NULL == pSubSection )
		m_section.clear();
	else
	{
		m_section = reg::section_rootOptions;
		stream::Tag( m_section, pSubSection, _T("\\") );		// append optional sub-section
	}
}

void CFolderOptions::LoadAll( void )
{
	if ( m_section.empty() )
		return;

	CWinApp* pApp = AfxGetApp();

	m_recurseFolders = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( RecurseFolders ), m_recurseFolders ) != FALSE;
	m_cutDuplicates = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( CutDuplicates ), m_cutDuplicates ) != FALSE;
	m_hideExt = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( HideExt ), m_hideExt ) != FALSE;
	m_rightJustifyExt = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( RightJustifyExt ), m_rightJustifyExt ) != FALSE;
	m_dirNamePrefix = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( DirNamePrefix ), m_dirNamePrefix ) != FALSE;
	m_sortFolders = pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( SortFolders ), m_sortFolders ) != FALSE;

	m_fileSortOrder.SetFromString( (LPCTSTR)pApp->GetProfileString( m_section.c_str(), ENTRY_OF( FileSortOrder ), m_fileSortOrder.GetAsString().c_str() ) );
	m_folderLayout = (FolderLayout)pApp->GetProfileInt( m_section.c_str(), ENTRY_OF( FolderLayout ), m_folderLayout );
}

void CFolderOptions::SaveAll( void ) const
{
	if ( m_section.empty() )
		return;

	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( RecurseFolders ), m_recurseFolders );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( CutDuplicates ), m_cutDuplicates );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( HideExt ), m_hideExt );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( RightJustifyExt ), m_rightJustifyExt );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( DirNamePrefix ), m_dirNamePrefix );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( SortFolders ), m_sortFolders );

	pApp->WriteProfileString( m_section.c_str(), ENTRY_OF( FileSortOrder ), m_fileSortOrder.GetAsString().c_str() );
	pApp->WriteProfileInt( m_section.c_str(), ENTRY_OF( FolderLayout ), m_folderLayout );
}

bool CFolderOptions::queryAddFileOrFolder( CString fileOrFolderPath, void* pFileOrFolder /*= NULL*/ )
{
	pFileOrFolder;
	if ( !m_cutDuplicates )
		return true;

	ASSERT( pFileOrFolder != NULL );
	// convert to standard path (back-slash and lower case)
	fileOrFolderPath = path::MakeNormal( fileOrFolderPath ).c_str();

	if ( m_fileDirMap.find( fileOrFolderPath ) != m_fileDirMap.end() )
		return false;

	m_fileDirMap.insert( fileOrFolderPath );
	return true;
}

DWORD CFolderOptions::getFlags( void ) const
{
	DWORD flags = 0;

	if ( m_recurseFolders ) flags |= foRecurseFolders;
	if ( m_cutDuplicates ) flags |= foCutDuplicates;
	if ( m_hideExt ) flags |= foHideExtension;
	if ( m_rightJustifyExt ) flags |= foRightJustifyExt;
	if ( m_dirNamePrefix ) flags |= foDirNamePrefix;
	if ( m_noOptionsPopup ) flags |= foNoOptionsPopup;
	if ( m_sortFolders ) flags |= foSortFolders;

	return flags;
}

void CFolderOptions::setFlags( DWORD flags )
{
	m_recurseFolders = ( flags & foRecurseFolders ) != 0;
	m_cutDuplicates = ( flags & foCutDuplicates ) != 0;
	m_hideExt = ( flags & foHideExtension ) != 0;
	m_rightJustifyExt = ( flags & foRightJustifyExt ) != 0;
	m_dirNamePrefix = ( flags & foDirNamePrefix ) != 0;
	m_noOptionsPopup = ( flags & foNoOptionsPopup ) != 0;

	m_sortFolders = ( flags & foSortFolders ) != 0;
}

std::tstring CFolderOptions::loadOptionsPopup( void )
{
	std::tstring popupText;
	m_optionsPopup.DestroyMenu();
	ui::LoadPopupMenu( m_optionsPopup, IDR_CONTEXT_MENU, app_popup::MenuBrowserOptions, ui::NormalMenuImages, &popupText );
	return popupText;
}

void CFolderOptions::EnableMenuCommands( void )
{
	if ( NULL == m_optionsPopup.GetSafeHmenu() )
		return;

	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_RECURSEFOLDERS, MF_BYCOMMAND | ( m_recurseFolders ? MF_CHECKED : MF_UNCHECKED ) );
	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_CUTDUPLICATES, MF_BYCOMMAND | ( m_cutDuplicates ? MF_CHECKED : MF_UNCHECKED ) );
	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_HIDEEXT, MF_BYCOMMAND | ( m_hideExt ? MF_CHECKED : MF_UNCHECKED ) );
	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_RIGHTJUSTIFYEXT, MF_BYCOMMAND | ( m_rightJustifyExt ? MF_CHECKED : MF_UNCHECKED ) );
	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_DIRNAMEPREFIX, MF_BYCOMMAND | ( m_dirNamePrefix ? MF_CHECKED : MF_UNCHECKED ) );

	m_optionsPopup.CheckMenuItem( CK_FILEBROWSER_SORTFOLDERS, MF_BYCOMMAND | ( m_sortFolders ? MF_CHECKED : MF_UNCHECKED ) );

	m_optionsPopup.CheckMenuRadioItem( CM_FILEBROWSER_FOLDERSASPOPUPS,
									   CM_FILEBROWSER_ALLFOLDERSEXPANDED,
									   CM_FILEBROWSER_FOLDERSASPOPUPS + m_folderLayout,
									   MF_BYCOMMAND );

	// update the sort order menu (last in 'm_optionsPopup')
	updateSortOrderMenu();
}


static struct CSortMenuLayout { PathField m_field; UINT m_id; } s_sortLayout[] =
{
	{ pfDrive, CK_FILEBROWSER_SORTBY_DRIVE },
	{ pfDir, CK_FILEBROWSER_SORTBY_DIR },
	{ pfDirName, CK_FILEBROWSER_SORTBY_DIRNAME },
	{ pfName, CK_FILEBROWSER_SORTBY_NAME },
	{ pfExt, CK_FILEBROWSER_SORTBY_EXT },
	{ pfFullPath, CK_FILEBROWSER_SORTBY_FULLPATH }
};

PathField CommandIdToPathFiled( UINT cmdId )
{
	for ( UINT i = 0; i != COUNT_OF( s_sortLayout ); ++i )
		if ( cmdId == s_sortLayout[ i ].m_id )
			return s_sortLayout[ i ].m_field;

	ASSERT( false );
	return (PathField)-1;
}

void CFolderOptions::updateSortOrderMenu( void )
{
	if ( m_optionsPopup.GetSafeHmenu() != NULL )
	{
		CMenu sortOrderPopup;

		sortOrderPopup.Attach( ::GetSubMenu( m_optionsPopup, m_optionsPopup.GetMenuItemCount() - 1 ) );
		updateSortOrderMenu( sortOrderPopup );
		sortOrderPopup.Detach();
	}
}

void CFolderOptions::updateSortOrderMenu( CMenu& rSortOrderPopup )
{
	for ( UINT i = 0; i != COUNT_OF( s_sortLayout ); ++i )
	{
		CString basicText;
		UINT newFlags = rSortOrderPopup.GetMenuState( s_sortLayout[ i ].m_id, MF_BYCOMMAND );

		rSortOrderPopup.GetMenuString( s_sortLayout[ i ].m_id, basicText, MF_BYCOMMAND );

		int suffixPos = basicText.ReverseFind( _T('(') );

		if ( suffixPos != -1 )
			basicText = basicText.Left( suffixPos - 1 );

		CString itemText;
		size_t foundPos = utl::FindPos( m_fileSortOrder.m_fields, s_sortLayout[ i ].m_field );
		if ( foundPos != std::tstring::npos )
		{
			itemText.Format( _T("%s\t(%d)"), (LPCTSTR)basicText, foundPos + 1 );
			newFlags |= MF_CHECKED;
		}
		else
		{
			itemText = basicText;
			newFlags &= ~MF_CHECKED;
		}

		rSortOrderPopup.ModifyMenu( s_sortLayout[ i ].m_id, MF_BYCOMMAND | newFlags, s_sortLayout[ i ].m_id, (LPCTSTR)itemText );
	}
}

bool CFolderOptions::OnMenuCommand( UINT cmdId )
{
	switch ( cmdId )
	{
		case CK_FILEBROWSER_RECURSEFOLDERS:
			m_recurseFolders = !m_recurseFolders;
			break;
		case CK_FILEBROWSER_CUTDUPLICATES:
			m_cutDuplicates = !m_cutDuplicates;
			break;
		case CK_FILEBROWSER_HIDEEXT:
			m_hideExt = !m_hideExt;
			break;
		case CK_FILEBROWSER_RIGHTJUSTIFYEXT:
			m_rightJustifyExt = !m_rightJustifyExt;
			break;
		case CK_FILEBROWSER_DIRNAMEPREFIX:
			m_dirNamePrefix = !m_dirNamePrefix;
			break;
		case CK_FILEBROWSER_SORTFOLDERS:
			m_sortFolders = !m_sortFolders;
			break;
		case CM_FILEBROWSER_FOLDERSASPOPUPS:
			m_folderLayout = flFoldersAsPopups;
			break;
		case CM_FILEBROWSER_FOLDERSASROOTPOPUPS:
			m_folderLayout = flFoldersAsRootPopups;
			break;
		case CM_FILEBROWSER_ROOTFOLDERSEXPANDED:
			m_folderLayout = flRootFoldersExpanded;
			break;
		case CM_FILEBROWSER_ALLFOLDERSEXPANDED:
			m_folderLayout = flAllFoldersExpanded;
			break;
		case CK_FILEBROWSER_SORTBY_DRIVE:
		case CK_FILEBROWSER_SORTBY_DIR:
		case CK_FILEBROWSER_SORTBY_DIRNAME:
		case CK_FILEBROWSER_SORTBY_NAME:
		case CK_FILEBROWSER_SORTBY_EXT:
		case CK_FILEBROWSER_SORTBY_FULLPATH:
			// if it's a file sort order command -> toggle sorting for specified field
			if ( !!ui::IsKeyPressed( VK_CONTROL ) )
				m_fileSortOrder.Clear();
			m_fileSortOrder.Toggle( CommandIdToPathFiled( cmdId ) );
			updateSortOrderMenu();	// so we can redraw it on right-click
			break;
		default:
			return false;
	}

	// (!) changes in COM object lifetime in VC 7.1 don't allow us to reliably save profile on destructor,
	// therefore we have to save pre-emptively.
	SaveAll();

	return true;
}

bool CFolderOptions::IsMenuStructureCommand( UINT cmdId )
{
	switch ( cmdId )
	{
		case CK_FILEBROWSER_RECURSEFOLDERS:
		case CK_FILEBROWSER_CUTDUPLICATES:
		case CK_FILEBROWSER_HIDEEXT:
		case CK_FILEBROWSER_RIGHTJUSTIFYEXT:
		case CK_FILEBROWSER_DIRNAMEPREFIX:
		case CK_FILEBROWSER_SORTFOLDERS:
		case CM_FILEBROWSER_FOLDERSASPOPUPS:
		case CM_FILEBROWSER_FOLDERSASROOTPOPUPS:
		case CM_FILEBROWSER_ROOTFOLDERSEXPANDED:
		case CM_FILEBROWSER_ALLFOLDERSEXPANDED:
			return true;
		default:
			return false;
	}
}

bool CFolderOptions::IsFilepathSortingCommand( UINT cmdId )
{
	switch ( cmdId )
	{
		case CK_FILEBROWSER_SORTBY_DRIVE:
		case CK_FILEBROWSER_SORTBY_DIR:
		case CK_FILEBROWSER_SORTBY_DIRNAME:
		case CK_FILEBROWSER_SORTBY_NAME:
		case CK_FILEBROWSER_SORTBY_EXT:
		case CK_FILEBROWSER_SORTBY_FULLPATH:
			return true;
		default:
			return false;
	}
}
