
#include "stdafx.h"
#include "PathItemListCtrl.h"
#include "PathItemBase.h"
#include "ShellContextMenuHost.h"
#include "Clipboard.h"
#include "PostCall.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
	, m_cmStyle( ExplorerSubMenu )
	, m_cmQueryFlags( CMF_EXPLORE )
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
	static CMenu s_stdPopupMenu[ _ListPopupCount ];
	CMenu& rMenu = s_stdPopupMenu[ popupType ];
	if ( NULL == rMenu.GetSafeHmenu() )
		ui::LoadPopupSubMenu( rMenu, IDR_STD_CONTEXT_MENU, ui::ListView, OnSelection == popupType ? lv::PathItemOnSelectionSubPopup : lv::PathItemNowhereSubPopup );
	return rMenu;
}

void CPathItemListCtrl::SetShellContextMenuStyle( ShellContextMenuStyle cmStyle, UINT cmQueryFlags /*= UINT_MAX*/ )
{
	m_cmStyle = cmStyle;

	if ( cmQueryFlags != UINT_MAX )
		m_cmQueryFlags = cmQueryFlags;
}

bool CPathItemListCtrl::IsInternalCmdId( int cmdId ) const
{
	if ( m_pShellMenuHost.get() != NULL )
		if ( m_pShellMenuHost->HasShellCmd( cmdId ) )
			return true;

	return __super::IsInternalCmdId( cmdId );
}

CMenu* CPathItemListCtrl::GetPopupMenu( ListPopup popupType )
{
	CMenu* pSrcPopupMenu = __super::GetPopupMenu( popupType );

	if ( pSrcPopupMenu != NULL && OnSelection == popupType && UseShellContextMenu() )
	{
		std::vector< std::tstring > selFilePaths;
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
		if ( m_pShellMenuHost.get() != NULL )
			m_pShellMenuHost->TrackMenu( pPopupMenu, screenPos, TPM_RIGHTBUTTON );
		else
			ui::TrackPopupMenu( *pPopupMenu, m_pTrackMenuTarget, screenPos );

		ui::PostCall( this, &CPathItemListCtrl::ResetShellContextMenu );		// delayed reset shell context tracker, after the command was handled or cancelled by user
		return true;		// handled
	}

	return false;
}

CMenu* CPathItemListCtrl::MakeContextMenuHost( CMenu* pSrcPopupMenu, const std::vector< std::tstring >& selFilePaths )
{
	REQUIRE( !selFilePaths.empty() );
	ASSERT_NULL( m_pShellMenuHost.get() );

	if ( ExplorerSubMenu == m_cmStyle && NULL == pSrcPopupMenu )
		m_cmStyle = ShellMenuLast;

	CComPtr< IContextMenu > pContextMenu = shell::MakeFilePathsContextMenu( selFilePaths, m_hWnd );
	if ( pContextMenu == NULL )
		return NULL;

	CMenu* pContextPopup = CMenu::FromHandle( ui::CloneMenu( pSrcPopupMenu != NULL ? pSrcPopupMenu->GetSafeHmenu() : ::CreatePopupMenu() ) );

	if ( ExplorerSubMenu == m_cmStyle )
	{
		m_pShellMenuHost.reset( new CShellLazyContextMenuHost( m_pTrackMenuTarget, selFilePaths, CMF_EXPLORE ) );	// lazy query the IContextMenu when expanding the "Explorer" sub-menu

		pContextPopup->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
		pContextPopup->AppendMenu( MF_BYPOSITION | MF_POPUP, (UINT_PTR)m_pShellMenuHost->GetPopupMenu()->GetSafeHmenu(), _T("E&xplorer") );
		ui::SetMenuItemImage( *pContextPopup, ui::CMenuItemRef::ByPosition( pContextPopup->GetMenuItemCount() - 1 ), ID_SHELL_SUBMENU );
		return pContextPopup;
	}

	m_pShellMenuHost.reset( new CShellContextMenuHost( m_pTrackMenuTarget ) );
	m_pShellMenuHost->Reset( pContextMenu );

	switch ( m_cmStyle )
	{
		case ShellMenuFirst:
			m_pShellMenuHost->MakePopupMenu( *pContextPopup, 0, m_cmQueryFlags );
			break;
		case ShellMenuLast:
			pContextPopup->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
			m_pShellMenuHost->MakePopupMenu( *pContextPopup, CShellContextMenuHost::AtEnd, m_cmQueryFlags );
			break;
	}

	if ( m_pShellMenuHost->HasShellCmds() )
		return pContextPopup;

	m_pShellMenuHost.reset();
	return NULL;
}

void CPathItemListCtrl::ResetShellContextMenu( void )
{
	m_pShellMenuHost.reset();
}

BOOL CPathItemListCtrl::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pShellMenuHost.get() != NULL )
		if ( m_pShellMenuHost->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CPathItemListCtrl, CReportListControl )
	ON_NOTIFY_REFLECT_EX( NM_DBLCLK, OnLvnDblclk_Reflect )
	ON_COMMAND( ID_ITEM_COPY_FILENAME, OnCopyFilename )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FILENAME, OnUpdateAnySelected )
	ON_COMMAND( ID_ITEM_COPY_PARENT_DIR_PATH, OnCopyParentDirPath )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_PARENT_DIR_PATH, OnUpdateAnySelected )
	ON_COMMAND( ID_EDIT_ITEM, OnEditFileProperties )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ITEM, OnUpdateAnySelected )
	ON_COMMAND( ID_EDIT_PROPERTIES, OnEditListViewProperties )
END_MESSAGE_MAP()

BOOL CPathItemListCtrl::OnLvnDblclk_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMITEMACTIVATE* pNmItemActivate = (NMITEMACTIVATE*)pNmHdr;
	*pResult = 0;

	UINT flags;
	int itemIndex = HitTest( pNmItemActivate->ptAction, &flags );
	if ( itemIndex != -1 && !HasFlag( flags, LVHT_ONITEMSTATEICON ) )				// on item but not checkbox
		if ( utl::ISubject* pCaretObject = GetSubjectAt( pNmItemActivate->iItem ) )
		{
			CShellContextMenuHost contextMenu( this );
			contextMenu.Reset( shell::MakeFilePathContextMenu( pCaretObject->GetCode(), m_hWnd ) );

			return contextMenu.InvokeDefaultVerb();
		}

	return FALSE;			// raise the notification to parent
}

void CPathItemListCtrl::OnCopyFilename( void )
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

void CPathItemListCtrl::OnCopyParentDirPath( void )
{
	std::vector< fs::CPath > selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	std::vector< fs::CPath > parentDirPaths;
	path::QueryParentPaths( parentDirPaths, selFilePaths );

	if ( !CClipboard::CopyToLines( parentDirPaths, this ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnEditFileProperties( void )
{
	std::vector< std::tstring > selFilePaths;
	QuerySelectedItemPaths( selFilePaths );
	ASSERT( !selFilePaths.empty() );

	CShellContextMenuHost contextMenu( this );
	contextMenu.Reset( shell::MakeFilePathsContextMenu( selFilePaths, m_hWnd ) );

	if ( !contextMenu.MakePopupMenu( CMF_NORMAL ) ||
		 !contextMenu.InvokeVerb( "properties" ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemListCtrl::OnEditListViewProperties( void )
{
AfxMessageBox( _T("TODO: OnEditListViewProperties!"), MB_OK );
}
