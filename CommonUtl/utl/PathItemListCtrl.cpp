
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


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
	, m_cmStyle( ExplorerSubMenu )
	, m_cmQueryFlags( CMF_EXPLORE )
{
	SetCustomFileGlyphDraw();
	SetPopupMenu( Nowhere, &GetStdPathListPopupMenu( Nowhere ) );
	SetPopupMenu( OnSelection, &GetStdPathListPopupMenu( OnSelection ) );

	AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
//m_cmStyle = ShellMenuFirst;
}

CPathItemListCtrl::~CPathItemListCtrl()
{
}

CMenu& CPathItemListCtrl::GetStdPathListPopupMenu( ListPopup popupType )
{
	static CMenu s_stdPopupMenu[ _ListPopupCount ];
	CMenu& rMenu = s_stdPopupMenu[ popupType ];
	if ( NULL == rMenu.GetSafeHmenu() )
		ui::LoadPopupSubMenu( rMenu, IDR_STD_CONTEXT_MENU, ui::ListView, OnSelection == popupType ? PathItemListOnSelectionSubPopup : PathItemListNowhereSubPopup );
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
		QuerySelectedItemPaths( selFilePaths );

		if ( !selFilePaths.empty() )
		{
			ASSERT_NULL( m_pShellMenuHost.get() );

			m_pShellMenuHost.reset( new CShellContextMenuHost( this ) );
			m_pShellMenuHost->Reset( shell::MakeFilePathsContextMenu( selFilePaths, m_hWnd ) );

			if ( m_pShellMenuHost->IsValid() )
			{
				CMenu* pContextPopup = CMenu::FromHandle( ui::CloneMenu( pSrcPopupMenu != NULL ? pSrcPopupMenu->GetSafeHmenu() : ::CreatePopupMenu() ) );

				ShellContextMenuStyle cmStyle = m_cmStyle;
				if ( ExplorerSubMenu == cmStyle && NULL == pSrcPopupMenu )
					cmStyle = ShellMenuLast;

				switch ( cmStyle )
				{
					case ExplorerSubMenu:
						if ( m_pShellMenuHost->MakePopupMenu( m_cmQueryFlags ) )
						{
							pContextPopup->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
							pContextPopup->AppendMenu( MF_BYPOSITION | MF_POPUP, (UINT_PTR)m_pShellMenuHost->GetPopupMenu()->Detach(), _T("E&xplorer") );
						}
						break;
					case ShellMenuFirst:
						m_pShellMenuHost->MakePopupMenu( *pContextPopup, 0, m_cmQueryFlags );
						break;
					case ShellMenuLast:
						m_pShellMenuHost->MakePopupMenu( *pContextPopup, CShellContextMenuHost::AtEnd, m_cmQueryFlags );
						break;
				}

				if ( m_pShellMenuHost->HasShellCmds() )
					return pContextPopup;
				else
					m_pShellMenuHost.reset();
			}
		}
	}

	return pSrcPopupMenu;
}

bool CPathItemListCtrl::TrackContextMenu( ListPopup popupType, const CPoint& screenPos )
{
	if ( CMenu* pPopupMenu = GetPopupMenu( popupType ) )
	{
		if ( m_pShellMenuHost.get() != NULL )
		{
			m_pShellMenuHost->TrackMenu( pPopupMenu, screenPos, TPM_RIGHTBUTTON );
			ui::PostCall( this, &CPathItemListCtrl::ResetShellContextMenu );		// delayed reset shell context tracker, after the command was handled or cancelled by user
		}
		else
			ui::TrackPopupMenu( *pPopupMenu, this, screenPos );

		return true;		// handled
	}

	return false;
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

	if ( utl::ISubject* pCaretObject = GetObjectAt( pNmItemActivate->iItem ) )
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

	fs::TPathSet parentDirPaths;
	for ( std::vector< fs::CPath >::const_iterator itSelFilePath = selFilePaths.begin(); itSelFilePath != selFilePaths.end(); ++itSelFilePath )
		parentDirPaths.insert( itSelFilePath->GetParentPath() );

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
