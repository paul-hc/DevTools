
#include "pch.h"
#include "PathItemListCtrl.h"
#include "PathItemBase.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "StdColors.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ReportListControl.hxx"


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
	, m_missingFileColor( color::ScarletRed )
{
	SetCustomFileGlyphDraw();
	SetPopupMenu( Nowhere, &GetStdPathListPopupMenu( Nowhere ) );
	SetPopupMenu( OnSelection, &GetStdPathListPopupMenu( OnSelection ) );

	AddRecordCompare( pred::NewComparator( pred::TCompareCode() ) );		// default row item comparator
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
	if ( nullptr == rMenu.GetSafeHmenu() )
		ui::LoadPopupMenu( &rMenu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::ListView, OnSelection == popupType ? lv::PathItemOnSelectionSubPopup : lv::PathItemNowhereSubPopup ) );
	return rMenu;
}

CMenu* CPathItemListCtrl::GetPopupMenu( ListPopup popupType )
{
	CMenu* pSrcPopupMenu = __super::GetPopupMenu( popupType );

	if ( pSrcPopupMenu != nullptr && OnSelection == popupType && UseShellContextMenu() )
	{
		std::vector<fs::CPath> selFilePaths;
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

void CPathItemListCtrl::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	ASSERT( this == pCtrl );

	if ( 0 == subItem && m_missingFileColor != CLR_NONE )
		if ( const utl::ISubject* pObject = AsPtr<utl::ISubject>( rowKey ) )
		{
			const fs::CPath filePath( pObject->GetCode() );

			if ( !filePath.FileExist() )
				rTextEffect.m_textColor = m_missingFileColor;		// highlight in red text the missing file/directory
		}

	__super::CombineTextEffectAt( rTextEffect, rowKey, subItem, pCtrl );
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

	if ( !ParentHandlesWmNotify( NM_DBLCLK ) )
	{
		*pResult = 0;

		UINT flags;
		int itemIndex = HitTest( pNmItemActivate->ptAction, &flags );
		if ( itemIndex != -1 && !HasFlag( flags, LVHT_ONITEMSTATEICON ) )				// on item but not checkbox
			if ( utl::ISubject* pCaretObject = GetSubjectAt( pNmItemActivate->iItem ) )
				return ShellInvokeDefaultVerb( std::vector<fs::CPath>( 1, pCaretObject->GetCode() ) );
	}

	return FALSE;			// raise the notification to parent
}

void CPathItemListCtrl::OnCopyFilenames( void )
{
	std::vector<std::tstring> selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	fs::CPath commonDirPath = path::ExtractCommonParentPath( selFilePaths );

	if ( !commonDirPath.IsEmpty() )
		for ( std::vector<std::tstring>::iterator itFilePath = selFilePaths.begin(); itFilePath != selFilePaths.end(); ++itFilePath )
			path::StripPrefix( *itFilePath, commonDirPath.GetPtr() );

	if ( !CTextClipboard::CopyToLines( selFilePaths, m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnCopyFolders( void )
{
	std::vector<fs::CPath> selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	std::vector<fs::CPath> parentDirPaths;
	path::QueryParentPaths( parentDirPaths, selFilePaths );

	if ( !CTextClipboard::CopyToLines( parentDirPaths, m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnFileProperties( void )
{
	std::vector<fs::CPath> selFilePaths;
	QuerySelectedItemPaths( selFilePaths );

	ShellInvokeProperties( selFilePaths );
}
