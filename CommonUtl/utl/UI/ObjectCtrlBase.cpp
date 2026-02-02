
#include "pch.h"
#include "ObjectCtrlBase.h"
#include "ShellPidl.h"
#include "ShellContextMenuHost.h"
#include "MenuUtilities.h"
#include "PostCall.h"
#include "WndUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CObjectCtrlBase::CObjectCtrlBase( CWnd* pCtrl, UINT ctrlAccelId /*= 0*/ )
	: m_pSubjectAdapter( nullptr )
	, m_pCtrl( pCtrl )
	, m_pTrackMenuTarget( m_pCtrl )
	, m_shCtxStyle( ExplorerSubMenu )
	, m_shCtxQueryFlags( CMF_EXPLORE )
{
	ASSERT_PTR( m_pCtrl );
	SetSubjectAdapter( ui::CDisplayCodeAdapter::Instance() );

	if ( ctrlAccelId != 0 )
		m_ctrlAccel.Load( ctrlAccelId );
}

CObjectCtrlBase::~CObjectCtrlBase()
{
}

void CObjectCtrlBase::SetSubjectAdapter( ui::ISubjectAdapter* pSubjectAdapter )
{
	ASSERT_PTR( pSubjectAdapter );
	m_pSubjectAdapter = pSubjectAdapter;
}

bool CObjectCtrlBase::IsInternalCmdId( int cmdId ) const
{
	if ( m_pShellMenuHost.get() != nullptr )
		if ( m_pShellMenuHost->HasShellCmd( cmdId ) )
			return true;

	return m_internalCmdIds.ContainsId( cmdId );
}

bool CObjectCtrlBase::HandleCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pShellMenuHost.get() != nullptr )
		if ( m_pShellMenuHost->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return true;

	return false;
}

bool CObjectCtrlBase::TranslateMessage( MSG* pMsg )
{
	return
		m_ctrlAccel.GetAccel() != nullptr &&
		m_ctrlAccel.Translate( pMsg, m_pTrackMenuTarget->m_hWnd, m_pCtrl->m_hWnd );
}


// shell context menu hosting/tracking

void CObjectCtrlBase::SetShellContextMenuStyle( ShellContextMenuStyle shCtxStyle, UINT shCtxQueryFlags /*= UINT_MAX*/ )
{
	m_shCtxStyle = shCtxStyle;

	if ( shCtxQueryFlags != UINT_MAX )
		m_shCtxQueryFlags = shCtxQueryFlags;
}

CMenu* CObjectCtrlBase::MakeContextMenuHost( CMenu* pSrcPopupMenu, const std::vector<shell::TPath>& shellPaths )
{
	REQUIRE( !shellPaths.empty() );
	ASSERT_NULL( m_pShellMenuHost.get() );

	if ( ExplorerSubMenu == m_shCtxStyle && nullptr == pSrcPopupMenu )
		m_shCtxStyle = ShellMenuLast;

	CComPtr<IContextMenu> pContextMenu = shell::MakeFilePathsContextMenu( shellPaths, m_pCtrl->m_hWnd );

	if ( nullptr == pContextMenu )
		return nullptr;

	CMenu* pContextPopup = CMenu::FromHandle( ui::CloneMenu( pSrcPopupMenu != nullptr ? pSrcPopupMenu->GetSafeHmenu() : ::CreatePopupMenu() ) );

	if ( ExplorerSubMenu == m_shCtxStyle )
	{
		m_pShellMenuHost.reset( new CShellLazyContextMenuHost( m_pTrackMenuTarget, pContextMenu, CMF_EXPLORE ) );	// lazy query the IContextMenu when expanding the "Explorer" sub-menu

		pContextPopup->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
		pContextPopup->AppendMenu( MF_BYPOSITION | MF_POPUP, (UINT_PTR)m_pShellMenuHost->GetPopupMenu()->GetSafeHmenu(), _T("E&xplorer") );
		ui::SetMenuItemImage( pContextPopup, ui::CMenuItemRef::ByPosition( pContextPopup->GetMenuItemCount() - 1 ), ID_SHELL_SUBMENU );
		return pContextPopup;
	}

	m_pShellMenuHost.reset( new CShellContextMenuHost( m_pTrackMenuTarget ) );
	m_pShellMenuHost->Reset( pContextMenu );

	switch ( m_shCtxStyle )
	{
		case ShellMenuFirst:
			m_pShellMenuHost->MakePopupMenu( *pContextPopup, 0, m_shCtxQueryFlags );
			break;
		case ShellMenuLast:
			pContextPopup->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
			m_pShellMenuHost->MakePopupMenu( *pContextPopup, CShellContextMenuHost::AtEnd, m_shCtxQueryFlags );
			break;
	}

	if ( m_pShellMenuHost->HasShellCmds() )
		return pContextPopup;

	m_pShellMenuHost.reset();
	return nullptr;
}

CMenu* CObjectCtrlBase::MakeContextMenuHost( CMenu* pSrcPopupMenu, const shell::TPath& shellPath )
{
	if ( shellPath.IsEmpty() )
		return nullptr;

	std::vector<shell::TPath> shellPaths( 1, shellPath );

	return MakeContextMenuHost( pSrcPopupMenu, shellPaths );
}

bool CObjectCtrlBase::DoTrackContextMenu( CMenu* pPopupMenu, const CPoint& screenPos )
{
	ASSERT_PTR( pPopupMenu->GetSafeHmenu() );

	int cmdId = 0;

	if ( m_pShellMenuHost.get() != nullptr )
		cmdId = m_pShellMenuHost->TrackMenu( pPopupMenu, screenPos, TPM_RIGHTBUTTON );
	else
		cmdId = ui::TrackPopupMenu( *pPopupMenu, m_pTrackMenuTarget, screenPos );

	ui::PostCall( this, &CObjectCtrlBase::ResetShellContextMenu );		// delayed reset shell context tracker, after the command was handled or cancelled by user
	return cmdId != 0;		// true if tracked
}

void CObjectCtrlBase::ResetShellContextMenu( void )
{
	m_pShellMenuHost.reset();
}

bool CObjectCtrlBase::ShellInvokeDefaultVerb( const std::vector<shell::TPath>& shellPaths )
{
	REQUIRE( !shellPaths.empty() );

	CShellContextMenuHost contextMenu( m_pCtrl );
	contextMenu.Reset( shell::MakeFilePathsContextMenu( shellPaths, m_pCtrl->m_hWnd ) );

	return contextMenu.InvokeDefaultVerb();
}

bool CObjectCtrlBase::ShellInvokeProperties( const std::vector<shell::TPath>& shellPaths )
{
	REQUIRE( !shellPaths.empty() );

	CShellContextMenuHost contextMenu( m_pCtrl );
	contextMenu.Reset( shell::MakeFilePathsContextMenu( shellPaths, m_pCtrl->m_hWnd ) );

	if ( contextMenu.MakePopupMenu( CMF_NORMAL ) )
		if ( contextMenu.InvokeVerb( "properties" ) )
			return true;

	ui::BeepSignal( MB_ICONWARNING );
	return false;
}
