
#include "stdafx.h"
#include "FileBrowser.h"
#include "IdeUtilities.h"
#include "TrackMenuWnd.h"
#include "Application.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/CmdUpdate.h"
#include "utl/FileSystem.h"
#include "utl/ContainerUtilities.h"
#include "utl/MenuUtilities.h"
#include "utl/ProcessUtils.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFolderItem implementation

CFolderItem::CFolderItem( const std::tstring& alias )
	: CPathItemBase( fs::CPath() )
	, m_alias( alias )
	, m_pParentFolder( NULL )
	, m_deepLeafCount( 0 )
{
}

CFolderItem::CFolderItem( CFolderItem* pParentFolder, const fs::CPath& folderPath, const std::tstring& wildSpecs, const std::tstring& alias )
	: CPathItemBase( folderPath )
	, m_wildSpecs( wildSpecs )
	, m_folderDirName( GetFilePath().GetNameExt() )
	, m_alias( !alias.empty() ? alias : m_folderDirName )
	, m_pParentFolder( pParentFolder )
	, m_deepLeafCount( 0 )
{
}

CFolderItem::CFolderItem( CFolderItem* pParentFolder, const fs::CPath& folderPath )
	: CPathItemBase( folderPath )
	, m_wildSpecs( pParentFolder->m_wildSpecs )
	, m_folderDirName( GetFilePath().GetNameExt() )
	, m_alias( m_folderDirName )
	, m_pParentFolder( safe_ptr( pParentFolder ) )
	, m_deepLeafCount( 0 )
{
	ASSERT( fs::IsValidDirectory( GetFilePath().GetPtr() ) );
}

CFolderItem::~CFolderItem()
{
	Clear();
}

void CFolderItem::Clear( void )
{
	utl::ClearOwningContainer( m_files );
	utl::ClearOwningContainer( m_subFolders );
}

std::tstring CFolderItem::FormatAlias( void ) const
{
	static std::tstring s_prefix( _T("+ ") );
	return s_prefix + m_alias;
}

void CFolderItem::SortFileItems( const CPathSortOrder& fileSortOrder )
{
	std::sort( m_files.begin(), m_files.end(), pred::MakeLessPtr( pred::CompareFileItem( &fileSortOrder ) ) );

	for ( std::vector< CFolderItem* >::const_iterator itSubFolder = m_subFolders.begin(); itSubFolder != m_subFolders.end(); ++itSubFolder )
		( *itSubFolder )->SortFileItems( fileSortOrder );
}

void CFolderItem::SortSubFolders( void )
{
	std::sort( m_subFolders.begin(), m_subFolders.end(), pred::MakeLessValue( pred::TCompareFolderItem() ) );
}

void CFolderItem::SearchForFiles( RecursionDepth depth, CPathIndex* pPathIndex )
{
	Clear();
	ASSERT( fs::IsValidDirectory( GetFilePath().GetPtr() ) );

	fs::CPathEnumerator found;
	fs::EnumFiles( &found, GetFilePath().GetPtr(), m_wildSpecs.c_str(), Shallow );

	for ( fs::TPathSet::const_iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); ++itFilePath )
		if ( NULL == pPathIndex || pPathIndex->RegisterUnique( *itFilePath ) )
			m_files.push_back( new CFileItem( this, *itFilePath ) );

	AddLeafCount( m_files.size() );

	if ( Deep == depth )
	{
		for ( fs::TPathSet::const_iterator itSubDirPath = found.m_subDirPaths.begin(); itSubDirPath != found.m_subDirPaths.end(); ++itSubDirPath )
			if ( NULL == pPathIndex || pPathIndex->RegisterUnique( *itSubDirPath ) )
			{
				std::auto_ptr< CFolderItem > pNewFolder( new CFolderItem( this, *itSubDirPath ) );

				pNewFolder->SearchForFiles( Deep, pPathIndex );
				if ( pNewFolder->HasAnyLeafs() )
					m_subFolders.push_back( pNewFolder.release() );
			}
	}
}

void CFolderItem::AddLeafCount( size_t leafCount )
{
	m_deepLeafCount += leafCount;

	for ( CFolderItem* pFolderItem = m_pParentFolder; pFolderItem != NULL; pFolderItem = pFolderItem->GetParentFolder() )
		pFolderItem->AddLeafCount( leafCount );
}

bool CFolderItem::AddFileItem( CPathIndex* pPathIndex, const fs::CPath& filePath, const std::tstring& fileLabel /*= std::tstring()*/ )
{
	if ( pPathIndex != NULL && !pPathIndex->RegisterUnique( filePath ) )
		return false;		// avoid global duplicates

	if ( FindFilePath( filePath ) != NULL )
		return false;		// avoid file duplicates in same folder

	m_files.push_back( new CFileItem( this, filePath, fileLabel ) );
	AddLeafCount( 1 );
	return true;
}

void CFolderItem::AddSubFolderItem( CFolderItem* pSubFolder )
{
	ASSERT_PTR( pSubFolder );
	ASSERT( this == pSubFolder->GetParentFolder() );
	ASSERT( !utl::Contains( m_subFolders, pSubFolder ) );

	m_subFolders.push_back( pSubFolder );
}

CFileItem* CFolderItem::FindFilePath( const fs::CPath& filePath ) const
{
	for ( std::vector< CFileItem* >::const_iterator itFile = m_files.begin(); itFile != m_files.end(); ++itFile )
		if ( ( *itFile )->GetFilePath() == filePath )
			return *itFile;

	return NULL;
}


// CFileItem implementation

CFileItem::CFileItem( const CFolderItem* pParentFolder, const fs::CPath& filePath, const std::tstring& label /*= std::tstring()*/ )
	: CPathItemBase( filePath )
	, m_pParentFolder( pParentFolder )
	, m_sortParts( GetFilePath() )
{
	fs::CPathParts labelParts( !label.empty() ? label : filePath.GetFilename() );
	m_labelName = labelParts.m_fname;
	m_labelExt = labelParts.m_ext;
}

CFileItem::~CFileItem()
{
}

bool CFileItem::HasFilePath( const fs::CPath& rightFilePath ) const
{
	if ( GetFilePath() == rightFilePath )
		return true;

	if ( path::EquivalentPtr( GetFilePath().GetNameExt(), rightFilePath.GetNameExt() ) )		// same filename?
		return path::IsRelative( rightFilePath.GetPtr() );

	return false;
}

std::tstring CFileItem::FormatLabel( const CFolderOptions* pOptions ) const
{
	ASSERT_PTR( pOptions );

	if ( pOptions->m_displayFullPath )
		return GetFilePath().Get();

	std::tstring text; text.reserve( GetFilePath().Get().size() );

	if ( pOptions->m_dirNamePrefix )
		text = m_sortParts.m_parentDirName.Get();

	stream::Tag( text, m_labelName, _T("/") );

	if ( !pOptions->m_hideExt )
		stream::Tag( text, m_labelExt, pOptions->m_rightJustifyExt ? _T("\t") : _T("") );

	return text;
}


namespace pred
{
	CompareResult CompareFileItem::operator()( const CFileItem* pLeft, const CFileItem* pRight ) const
	{
		const std::vector< PathField >& fields = m_pSortOrder->GetFields();
		CompareResult result = Equal;

		for ( std::vector< PathField >::const_iterator itField = fields.begin(); Equal == result && itField != fields.end(); ++itField )
			result = ComparePathField( pLeft->GetSortParts(), pRight->GetSortParts(), *itField );

		return result;
	}
}


// CFolderOptions implementation

namespace reg
{
	static const TCHAR section_folderOptions[] = _T("FolderOptions");
}

CFolderOptions::CFolderOptions( const TCHAR* pSubSection /*= _T("")*/ )
	: CRegistryOptions( std::tstring(), CRegistryOptions::SaveAllOnModify )
	, m_recurseFolders( false )
	, m_cutDuplicates( false )
	, m_displayFullPath( false )
	, m_hideExt( false )
	, m_rightJustifyExt( false )
	, m_dirNamePrefix( true )
	, m_noOptionsPopup( false )
	, m_sortFolders( true )
	, m_folderLayout( flFoldersAsPopups )
{
	AddOption( MAKE_OPTION( &m_recurseFolders ), CK_FILEBROWSER_RECURSEFOLDERS );
	AddOption( MAKE_OPTION( &m_cutDuplicates ), CK_FILEBROWSER_CUTDUPLICATES );
	AddOption( MAKE_OPTION( &m_displayFullPath ), CK_FILEBROWSER_DISPLAYFULLPATH );
	AddOption( MAKE_OPTION( &m_hideExt ), CK_FILEBROWSER_HIDEEXT );
	AddOption( MAKE_OPTION( &m_rightJustifyExt ), CK_FILEBROWSER_RIGHTJUSTIFYEXT );
	AddOption( MAKE_OPTION( &m_dirNamePrefix ), CK_FILEBROWSER_DIRNAMEPREFIX );
	AddOption( MAKE_OPTION( &m_sortFolders ), CK_FILEBROWSER_SORTFOLDERS );

	reg::CEnumOption* pFolderLayoutOption = MAKE_ENUM_OPTION( &m_folderLayout );
	pFolderLayoutOption->SetRadioIds( CM_FILEBROWSER_FoldersAsPopups, CM_FILEBROWSER_AllFoldersExpanded );
	AddOption( pFolderLayoutOption );

	AddOption( reg::MakeOption( m_fileSortOrder.GetOrderTextPtr(), _T("SortOrder") ) );

	SetSubSection( pSubSection );			// will LoadAll() if persistent
}

CFolderOptions::~CFolderOptions()
{
}

void CFolderOptions::SetSubSection( const TCHAR* pSubSection )
{
	if ( NULL == pSubSection )
		SetSection( NULL );
	else
	{
		std::tstring section = reg::section_folderOptions;
		stream::Tag( section, pSubSection, _T("\\") );		// append optional sub-section

		SetSectionName( section );
	}

	if ( IsPersistent() )
		LoadAll();
}

void CFolderOptions::LoadAll( void )
{
	std::tstring origSortOrderText = m_fileSortOrder.GetOrderText();

	CRegistryOptions::LoadAll();

	if ( m_fileSortOrder.GetOrderText() != origSortOrderText )		// changed indirect value?
		m_fileSortOrder.ParseOrderText();
}

DWORD CFolderOptions::GetFlags( void ) const
{
	DWORD flags = 0;

	SetFlag( flags, foRecurseFolders, m_recurseFolders );
	SetFlag( flags, foCutDuplicates, m_cutDuplicates );
	SetFlag( flags, foHideExtension, m_hideExt );
	SetFlag( flags, foRightJustifyExt, m_rightJustifyExt );
	SetFlag( flags, foDirNamePrefix, m_dirNamePrefix );
	SetFlag( flags, foNoOptionsPopup, m_noOptionsPopup );
	SetFlag( flags, foSortFolders, m_sortFolders );

	return flags;
}

void CFolderOptions::SetFlags( DWORD flags )
{
	m_recurseFolders = HasFlag( flags, foRecurseFolders );
	m_cutDuplicates = HasFlag( flags, foCutDuplicates );
	m_hideExt = HasFlag( flags, foHideExtension );
	m_rightJustifyExt = HasFlag( flags, foRightJustifyExt );
	m_dirNamePrefix = HasFlag( flags, foDirNamePrefix );
	m_noOptionsPopup = HasFlag( flags, foNoOptionsPopup );
	m_sortFolders = HasFlag( flags, foSortFolders );
}

static struct CSortMenuLayout { PathField m_field; UINT m_cmdId; } s_fieldToId[] =
{
	{ pfDrive, CM_FILEBROWSER_SortByDrive },
	{ pfDir, CM_FILEBROWSER_SortByDirPath },
	{ pfDirName, CM_FILEBROWSER_SortByDirName },
	{ pfName, CM_FILEBROWSER_SortByFname },
	{ pfExt, CM_FILEBROWSER_SortByExt },
	{ pfFullPath, CM_FILEBROWSER_SortByFullPath }
};

PathField CFolderOptions::PathFieldFromCmd( UINT cmdId )
{
	for ( UINT i = 0; i != COUNT_OF( s_fieldToId ); ++i )
		if ( s_fieldToId[ i ].m_cmdId == cmdId )
			return s_fieldToId[ i ].m_field;

	ASSERT( false );
	return (PathField)-1;
}

UINT CFolderOptions::CmdFromPathField( PathField field )
{
	for ( UINT i = 0; i != COUNT_OF( s_fieldToId ); ++i )
		if ( s_fieldToId[ i ].m_field == field )
			return s_fieldToId[ i ].m_cmdId;

	ASSERT( false );
	return 0;
}


// CFolderOptions command handlers

BEGIN_MESSAGE_MAP( CFolderOptions, CRegistryOptions )
	ON_COMMAND_RANGE( CM_FILEBROWSER_SortByDrive, CM_FILEBROWSER_SortByFullPath, OnToggle_SortBy )
	ON_UPDATE_COMMAND_UI_RANGE( CM_FILEBROWSER_SortByDrive, CM_FILEBROWSER_SortByFullPath, OnUpdate_SortBy )
	ON_COMMAND( ID_RESET_DEFAULT, OnResetSortOrder )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdateResetSortOrder )
END_MESSAGE_MAP()

void CFolderOptions::OnUpdateOption( CCmdUI* pCmdUI )
{
	CRegistryOptions::OnUpdateOption( pCmdUI );

	switch ( pCmdUI->m_nID )
	{
		case CK_FILEBROWSER_HIDEEXT:
		case CK_FILEBROWSER_RIGHTJUSTIFYEXT:
		case CK_FILEBROWSER_DIRNAMEPREFIX:
			pCmdUI->Enable( !m_displayFullPath );
			break;
	}
}

void CFolderOptions::OnToggle_SortBy( UINT cmdId )
{
	if ( !ui::IsKeyPressed( VK_CONTROL ) )
		m_fileSortOrder.Clear();

	m_fileSortOrder.Toggle( PathFieldFromCmd( cmdId ) );

	OnOptionChanged( m_fileSortOrder.GetOrderTextPtr() );
}

void CFolderOptions::OnUpdate_SortBy( CCmdUI* pCmdUI )
{
	std::tstring itemText = ui::GetCmdText( pCmdUI );

	size_t sepPos = itemText.find( _T('\t') );
	std::tstring newItemText = sepPos != std::tstring::npos ? itemText.substr( 0, sepPos ) : itemText;

	size_t fieldPos = m_fileSortOrder.FindFieldPos( PathFieldFromCmd( pCmdUI->m_nID ) );
	if ( fieldPos != utl::npos )
		stream::Tag( newItemText, str::Format( _T("(%d)"), fieldPos + 1 ), _T("\t") );

	if ( newItemText != itemText )
		pCmdUI->SetText( newItemText.c_str() );

	pCmdUI->SetCheck( fieldPos != utl::npos );
}

void CFolderOptions::OnResetSortOrder( void )
{
	m_fileSortOrder.ResetDefaultOrder();
	OnOptionChanged( m_fileSortOrder.GetOrderTextPtr() );
}

void CFolderOptions::OnUpdateResetSortOrder( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_fileSortOrder.IsDefaultOrder() );
}


// CTargetMenu implementation

void CTargetMenu::InitInplace( CMenu* pParentMenu )
{
	REQUIRE( NULL == m_menu.GetSafeHmenu() );

	ASSERT_PTR( pParentMenu->GetSafeHmenu() );
	m_menu.Attach( pParentMenu->GetSafeHmenu() );
	m_initialCount = pParentMenu->GetMenuItemCount();
}

void CTargetMenu::InitAsSubMenu( const std::tstring& subMenuCaption )
{
	REQUIRE( NULL == m_menu.GetSafeHmenu() );

	m_menu.CreatePopupMenu();
	m_initialCount = 0;
	m_subMenuCaption = subMenuCaption;
}

void CTargetMenu::Commit( CMenu* pParentMenu )
{
	ASSERT_PTR( pParentMenu->GetSafeHmenu() );

	if ( IsSubMenu() )
	{
		if ( !IsEmpty() )
			AppendSubMenu( pParentMenu, m_menu.Detach(), m_subMenuCaption );
	}
	else
	{
		ASSERT( pParentMenu->GetSafeHmenu() == m_menu.GetSafeHmenu() );

		if ( !IsEmpty() )
			AppendSeparator();

		m_menu.Detach();		// release ownership
	}
}

bool CTargetMenu::AppendSeparator( CMenu* pMenu )
{
	ASSERT_PTR( pMenu->GetSafeHmenu() );

	UINT itemCount = pMenu->GetMenuItemCount();
	if ( 0 == itemCount || 0 == pMenu->GetMenuItemID( itemCount - 1 ) )		// empty menu or last item is a separator?
		return false;

	return DoAppendItem( pMenu, MF_SEPARATOR );
}

bool CTargetMenu::AppendContextSubMenu( CMenu* pMenu, app::ContextPopup popupIndex )
{
	ASSERT_PTR( pMenu->GetSafeHmenu() );

	CMenu subMenu;
	std::tstring subMenuCaption;
	ui::LoadPopupMenu( subMenu, IDR_CONTEXT_MENU, popupIndex, ui::NormalMenuImages, &subMenuCaption );

	return AppendSubMenu( pMenu, subMenu.Detach(), subMenuCaption );
}

bool CTargetMenu::DoAppendItem( CMenu* pMenu, UINT flags, UINT_PTR itemId /*= 0*/, const TCHAR* pItemText /*= NULL*/ )
{
	ASSERT_PTR( pMenu->GetSafeHmenu() );

	UINT itemCount = pMenu->GetMenuItemCount();

	if ( HasFlag( flags, MF_SEPARATOR ) )
		if ( 0 == itemCount || 0 == pMenu->GetMenuItemID( itemCount - 1 ) )
			return false;

	UINT maxVertItems = app::GetMenuVertSplitCount();

	if ( itemCount != 0 && 0 == ( itemCount % maxVertItems ) )
		flags |= MF_MENUBARBREAK | MF_MENUBREAK;

	return pMenu->AppendMenu( flags, itemId, pItemText ) != FALSE;
}


// CFileMenuBuilder implementation

CFileMenuBuilder::CFileMenuBuilder( const CFolderOptions* pOptions )
	: m_pOptions( pOptions )
	, m_fileItemId( CM_FILEBROWSER_FILEITEM )
{
	ASSERT_PTR( m_pOptions );
	m_rootPopupMenu.CreatePopupMenu();
}

const CFileItem* CFileMenuBuilder::FindFileItemWithId( UINT cmdId ) const
{
	TMapIdToItem::const_iterator itFound = m_idToItemMap.find( cmdId );
	return itFound != m_idToItemMap.end() ? itFound->second : NULL;
}

UINT CFileMenuBuilder::MarkCurrFileItemId( const fs::CPath& currFilePath )
{
	for ( UINT i = 0, count = m_rootPopupMenu.GetMenuItemCount(); i != count; ++i )
		if ( !HasFlag( m_rootPopupMenu.GetMenuState( i, MF_BYPOSITION ), MF_POPUP | MF_SEPARATOR | MF_MENUBREAK | MFT_OWNERDRAW ) )
		{
			UINT cmdId = m_rootPopupMenu.GetMenuItemID( i );
			ASSERT( cmdId != 0 && cmdId != UINT_MAX );

			if ( const CFileItem* pFileItem = FindFileItemWithId( cmdId ) )
				if ( pFileItem->HasFilePath( currFilePath ) )
				{
					m_rootPopupMenu.SetDefaultItem( cmdId );
					return cmdId;
				}
		}

	return 0;
}

bool CFileMenuBuilder::BuildFolderItem( CMenu* pParentMenu, const CFolderItem* pFolderItem )
{
	ASSERT_PTR( pParentMenu->GetSafeHmenu() );
	ASSERT_PTR( pFolderItem );

	if ( !pFolderItem->HasAnyLeafs() )
		return false;					// avoid adding empty folders

	CScopedTargetMenu scopedTargetMenu( pParentMenu );

	if ( UseSubMenu( pFolderItem ) )
		scopedTargetMenu.InitAsSubMenu( pFolderItem->FormatAlias() );
	else
		scopedTargetMenu.InitInplace( pParentMenu );

	AppendFolderItem( &scopedTargetMenu, pFolderItem );
	return true;
}

bool CFileMenuBuilder::UseSubMenu( const CFolderItem* pFolderItem ) const
{
	if ( pFolderItem->IsRootFolder() )
		return false;							// always expand files of the root folder

	switch ( m_pOptions->m_folderLayout )
	{
		default: ASSERT( false );
		case flFoldersAsPopups:
		case flFoldersAsRootPopups:
			return true;
		case flTopFoldersExpanded:
			return !pFolderItem->IsTopFolder();
		case flAllFoldersExpanded:
			return false;
	}
}

void CFileMenuBuilder::AppendFolderItem( CTargetMenu* pTargetMenu, const CFolderItem* pFolderItem )
{
	AppendFileItems( pTargetMenu, pFolderItem->GetFileItems() );
	AppendSubFolders( pTargetMenu, pFolderItem->GetSubFolders() );
}

void CFileMenuBuilder::AppendSubFolders( CTargetMenu* pTargetMenu, const std::vector< CFolderItem* >& subFolders )
{
	for ( std::vector< CFolderItem* >::const_iterator itSubFolder = subFolders.begin(); itSubFolder != subFolders.end(); ++itSubFolder )
		BuildFolderItem( pTargetMenu->GetMenu(), *itSubFolder );
}

void CFileMenuBuilder::AppendFileItems( CTargetMenu* pTargetMenu, const std::vector< CFileItem* >& fileItems )
{
	for ( std::vector< CFileItem* >::const_iterator itFileItem = fileItems.begin(); itFileItem != fileItems.end(); ++itFileItem )
		if ( RegisterMenuUniqueItem( pTargetMenu->GetMenu(), *itFileItem ) )		// unique path item in the popup?
		{
			UINT cmdId = GetNextFileItemId();

			pTargetMenu->AppendItem( cmdId, ( *itFileItem )->FormatLabel( m_pOptions ) );
			m_idToItemMap[ cmdId ] = *itFileItem;
		}
}

bool CFileMenuBuilder::RegisterMenuUniqueItem( const CMenu* pPopupMenu, const CFileItem* pFileItem )
{
	ASSERT_PTR( pPopupMenu->GetSafeHmenu() );
	ASSERT_PTR( pFileItem );

	const CFileItem*& rpItem = m_menuPathToItemMap[ TMenuPathKey( pPopupMenu->GetSafeHmenu(), pFileItem->GetFilePath() ) ];
	if ( rpItem != NULL )
		return false;				// reject duplicate path menu items in the same popup menu

	rpItem = pFileItem;
	return true;
}


// CFileBrowser implementation

CFileBrowser::CFileBrowser( void )
	: m_options()
	, m_pRootFolderItem( new CFolderItem( _T("[Root Files]") ) )
{
	if ( m_options.m_cutDuplicates )
		m_pPathIndex.reset( new CPathIndex );
}

CFileBrowser::~CFileBrowser()
{
}

bool CFileBrowser::AddFolder( const fs::CPath& folderPathFilter, const std::tstring& folderAlias )
{
	fs::CPath folderPath = folderPathFilter.GetParentPath();

	if ( !fs::IsValidDirectory( folderPath.GetPtr() ) )
		return false;

	if ( m_pPathIndex.get() != NULL && !m_pPathIndex->RegisterUnique( folderPathFilter ) )
		return false;

	std::auto_ptr< CFolderItem > pNewFolder( new CFolderItem( m_pRootFolderItem.get(), folderPath, folderPathFilter.GetFilename(), folderAlias ) );

	pNewFolder->SearchForFiles( m_options.m_recurseFolders ? Deep : Shallow, m_pPathIndex.get() );
	if ( !pNewFolder->HasAnyLeafs() )
		return false;

	m_pRootFolderItem->AddSubFolderItem( pNewFolder.release() );
	return true;
}

bool CFileBrowser::AddFolderItems( const TCHAR* pFolderItems )
{
	// pFolderItems: folder items separated by EDIT_SEP=";" token (at end no token);
	// Each folder item contains a folder path filter, and optional, a folder alias separated
	// by PROF_SEP="|" separator.
	// example:
	//	"D:\Development\Tools\Slider\LogicalFilesBackup.h,LogicalFilesBackup.hxx,LogicalFilesBackup.cxx|ASSOCIATIONS;D:\Development\Tools\Slider\slider Desc\LogicalFilesBackup_Factory.*,LogicalFilesBackup_Tags.*|FACTORY"

	std::vector< std::tstring > dualArray;
	str::Split( dualArray, pFolderItems, _T(";") );

	for ( std::vector< std::tstring >::const_iterator itFolderItem = dualArray.begin(); itFolderItem != dualArray.end(); ++itFolderItem )
	{
		ASSERT( !itFolderItem->empty() );
		size_t sepPos = itFolderItem->find( PROF_SEP );

		if ( sepPos != std::tstring::npos )
			AddFolder( itFolderItem->substr( 0, sepPos ), itFolderItem->substr( sepPos + 1 ) );			// alias is specified
		else
			AddFolder( *itFolderItem, std::tstring() );		// no alias
	}

	return !dualArray.empty();
}

bool CFileBrowser::AddRootFile( const fs::CPath& filePath, const std::tstring& label /*= std::tstring()*/ )
{
	return m_pRootFolderItem->AddFileItem( m_pPathIndex.get(), filePath, label );
}

void CFileBrowser::SortItems( void )
{
	ASSERT_PTR( m_pRootFolderItem.get() );

	if ( m_options.m_sortFolders )
		m_pRootFolderItem->SortSubFolders();

	m_pRootFolderItem->SortFileItems( m_options.m_fileSortOrder );
}

CMenu* CFileBrowser::BuildMenu( void )
{
	m_pTrackInfo.reset( new CTrackInfo( &m_options ) );
	m_pTrackInfo->m_menuBuilder.BuildFolderItem( m_pRootFolderItem.get() );

	CMenu* pPopupMenu = m_pTrackInfo->m_menuBuilder.GetPopupMenu();

	if ( !m_options.m_noOptionsPopup )
	{	// append options popups
		CTargetMenu::AppendSeparator( pPopupMenu );
		CTargetMenu::AppendContextSubMenu( pPopupMenu, app::MenuBrowserOptionsPopup );
		CTargetMenu::AppendContextSubMenu( pPopupMenu, app::FileSortOrderPopup );
	}

	ui::DeleteLastMenuSeparator( *pPopupMenu );

	ENSURE( ui::IsValidMenu( pPopupMenu->GetSafeHmenu() ) );
	return pPopupMenu;
}

bool CFileBrowser::PickFile( CPoint screenPos )
{
	if ( -1 == screenPos.x && -1 == screenPos.y )
		screenPos = ui::GetCursorPos();

	SortItems();

	ide::CScopedWindow scopedIDE;
	if ( !scopedIDE.IsValid() )
		return false;

	CTrackMenuWnd trackingWnd( this );

	VERIFY( trackingWnd.Create( scopedIDE.GetFocusWnd() ) );
	trackingWnd.SetRightClickRepeat();		// keep popup menu open when right clicking on a command

	for ( ;; )
	{
		CMenu* pPopupMenu = BuildMenu();

		trackingWnd.SetHilightId( GetMenuBuilder()->MarkCurrFileItemId( m_currFilePath ) );

		if ( trackingWnd.TrackContextMenu( pPopupMenu, screenPos ) != 0 )
			DEBUG_LOG( _T("Picked cmdId=%d, filePath: %s"), trackingWnd.GetSelCmdId(), GetCurrFilePath().GetPtr() );

		if ( !m_pTrackInfo->m_keepTracking )
			break;
	}

	m_pTrackInfo.reset();

	trackingWnd.DestroyWindow();

	return IsFileCmd( trackingWnd.GetSelCmdId() ) && !m_currFilePath.IsEmpty();
}

bool CFileBrowser::IsFileCmd( UINT cmdId )
{
	return cmdId >= CM_FILEBROWSER_FILEITEM && cmdId <= CM_FILEBROWSER_FILEITEM_LAST;
}

BOOL CFileBrowser::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_options.OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
	{
		if ( CN_COMMAND == code && NULL == pHandlerInfo && m_pTrackInfo.get() != NULL )
			m_pTrackInfo->m_keepTracking = true;		// keep tracking the context menu while options are changed

		return TRUE;
	}

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// CFileBrowser command handlers

BEGIN_MESSAGE_MAP( CFileBrowser, CCmdTarget )
	ON_COMMAND_RANGE( CM_FILEBROWSER_FILEITEM, CM_FILEBROWSER_FILEITEM_LAST, OnCommand_FileItem )
	ON_UPDATE_COMMAND_UI_RANGE( CM_FILEBROWSER_FILEITEM, CM_FILEBROWSER_FILEITEM_LAST, OnUpdate_FileItem )
END_MESSAGE_MAP()

void CFileBrowser::OnCommand_FileItem( UINT cmdId )
{
	if ( CFileMenuBuilder* pMenuBuilder = GetMenuBuilder() )
		if ( const CFileItem* pPickedFileItem = pMenuBuilder->FindFileItemWithId( cmdId ) )
			if ( ui::IsKeyPressed( VK_CONTROL ) )
			{	// final command with CTRL pressed -> sent selected file path to the clipboard
				if ( CClipboard::CopyText( pPickedFileItem->GetFilePath().Get() ) )
					SetCurrFilePath( fs::CPath() );			// reset the picked file path member in order to avoid the final execution
			}
			else
				SetCurrFilePath( pPickedFileItem->GetFilePath() );
}

void CFileBrowser::OnUpdate_FileItem( CCmdUI* pCmdUI )
{
	if ( CFileMenuBuilder* pMenuBuilder = GetMenuBuilder() )
		if ( const CFileItem* pPickedFileItem = pMenuBuilder->FindFileItemWithId( pCmdUI->m_nID ) )
			if ( pPickedFileItem->HasFilePath( m_currFilePath ) )
				ui::SetRadio( pCmdUI );
}
