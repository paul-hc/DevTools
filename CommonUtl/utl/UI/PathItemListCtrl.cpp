
#include "stdafx.h"
#include "PathItemListCtrl.h"
#include "PathItemBase.h"
#include "Clipboard.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "Utilities.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ReportListControl.hxx"


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
{
	SetCustomFileGlyphDraw();
	SetPopupMenu( Nowhere, &GetStdPathListPopupMenu( Nowhere ) );
	SetPopupMenu( OnSelection, &GetStdPathListPopupMenu( OnSelection ) );

	AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
}

CPathItemListCtrl::~CPathItemListCtrl()
{
}

CMenu& CPathItemListCtrl::GetStdPathListPopupMenu( ListPopup popupType )
{
	if ( OnGroup == popupType )
		return __super::GetStdPopupMenu( popupType );

	static CMenu s_stdPopupMenu[ _ListPopupCount ];
	CMenu& rMenu = s_stdPopupMenu[ popupType ];
	if ( NULL == rMenu.GetSafeHmenu() )
		ui::LoadPopupSubMenu( rMenu, IDR_STD_CONTEXT_MENU, ui::ListView, OnSelection == popupType ? lv::PathItemOnSelectionSubPopup : lv::PathItemNowhereSubPopup );
	return rMenu;
}

CMenu* CPathItemListCtrl::GetPopupMenu( ListPopup popupType )
{
	CMenu* pSrcPopupMenu = __super::GetPopupMenu( popupType );

	if ( pSrcPopupMenu != NULL && OnSelection == popupType && UseShellContextMenu() )
	{
		std::vector< fs::CPath > selFilePaths;
		if ( QuerySelectedItemPaths( selFilePaths ) )
			if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, selFilePaths ) )
				return pContextPopup;
	}

	return pSrcPopupMenu;
}

bool CPathItemListCtrl::TrackContextMenu( ListPopup popupType, const CPoint& screenPos )
{
	if ( CMenu* pPopupMenu = GetPopupMenu( popupType ) )
	{
		DoTrackContextMenu( pPopupMenu, screenPos );
		return true;		// handled
	}

	return false;
}

BOOL CPathItemListCtrl::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( HandleCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CPathItemListCtrl, CReportListControl )
	ON_NOTIFY_REFLECT_EX( NM_DBLCLK, OnLvnDblclk_Reflect )
	ON_COMMAND( ID_ITEM_COPY_FILENAMES, OnCopyFilenames )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FILENAMES, OnUpdateAnySelected )
	ON_COMMAND( ID_ITEM_COPY_FOLDERS, OnCopyFolders )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FOLDERS, OnUpdateAnySelected )
	ON_COMMAND( ID_FILE_PROPERTIES, OnFileProperties )
	ON_UPDATE_COMMAND_UI( ID_FILE_PROPERTIES, OnUpdateAnySelected )
END_MESSAGE_MAP()

BOOL CPathItemListCtrl::OnLvnDblclk_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMITEMACTIVATE* pNmItemActivate = (NMITEMACTIVATE*)pNmHdr;
	*pResult = 0;

	UINT flags;
	int itemIndex = HitTest( pNmItemActivate->ptAction, &flags );
	if ( itemIndex != -1 && !HasFlag( flags, LVHT_ONITEMSTATEICON ) )				// on item but not checkbox
		if ( utl::ISubject* pCaretObject = GetSubjectAt( pNmItemActivate->iItem ) )
			return ShellInvokeDefaultVerb( std::vector< fs::CPath >( 1, pCaretObject->GetCode() ) );

	return FALSE;			// raise the notification to parent
}

void CPathItemListCtrl::OnCopyFilenames( void )
{
	std::vector< std::tstring > selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	fs::CPath commonDirPath = path::ExtractCommonParentPath( selFilePaths );

	if ( !commonDirPath.IsEmpty() )
		for ( std::vector< std::tstring >::iterator itFilePath = selFilePaths.begin(); itFilePath != selFilePaths.end(); ++itFilePath )
			path::StripPrefix( *itFilePath, commonDirPath.GetPtr() );

	if ( !CClipboard::CopyToLines( selFilePaths, this ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnCopyFolders( void )
{
	std::vector< fs::CPath > selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	std::vector< fs::CPath > parentDirPaths;
	path::QueryParentPaths( parentDirPaths, selFilePaths );

	if ( !CClipboard::CopyToLines( parentDirPaths, this ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnFileProperties( void )
{
	std::vector< fs::CPath > selFilePaths;
	QuerySelectedItemPaths( selFilePaths );

	ShellInvokeProperties( selFilePaths );
}
