
#include "stdafx.h"
#include "PathItemListCtrl.h"
#include "PathItemBase.h"
#include "MenuUtilities.h"
#include "PostCall.h"
#include "ShellContextMenuHost.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemListCtrl::CPathItemListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
	, m_cmStyle( ExplorerSubMenu )
	, m_cmQueryFlags( CMF_EXPLORE )
{
	AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
	SetCustomFileGlyphDraw();
//m_cmStyle = ShellMenuFirst;
}

CPathItemListCtrl::~CPathItemListCtrl()
{
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
				ui::SetMenuImages( *pContextPopup );

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
		if ( NULL == m_pShellMenuHost.get() )
			ui::TrackPopupMenu( *pPopupMenu, this, screenPos );
		else
		{
			m_pShellMenuHost->TrackMenu( pPopupMenu, screenPos, TPM_RIGHTBUTTON );
			ui::PostCall( this, &CPathItemListCtrl::ResetShellContextMenu );		// delayed reset shell context tracker, after the command was handled or cancelled by user
		}
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
