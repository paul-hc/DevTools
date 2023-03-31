
#include "pch.h"
#include "PopupDlgBase.h"
#include "AboutBox.h"
#include "AccelTable.h"
#include "ImageStore.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include "VersionInfo.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static ACCEL keys[] =
{
	{ FVIRTKEY, VK_F1, ID_APP_ABOUT }
};


// CPopupDlgBase implementation

CPopupDlgBase::CPopupDlgBase( void )
	: m_resizable( true )
	, m_initCentered( true )
	, m_hideSysMenuIcon( true )
	, m_noAboutMenuItem( false )
	, m_modeless( false )
	, m_autoDelete( false )
	, m_isTopDlg( false )
	, m_idleUpdateDeep( false )
	, m_dlgIconId( 0 )
{
	m_accelPool.AddAccelTable( new CAccelTable( keys, COUNT_OF( keys ) ) );
}

const CIcon* CPopupDlgBase::GetDlgIcon( DlgIcon dlgIcon /*= DlgSmallIcon*/ ) const
{
	if ( m_dlgIconId != 0 )
		return ui::GetImageStoresSvc()->RetrieveIcon( CIconId( m_dlgIconId, DlgSmallIcon == dlgIcon ? SmallIcon : LargeIcon ) );
	return nullptr;
}

void CPopupDlgBase::LoadDlgIcon( UINT dlgIconId )
{
	// normally called prior to creation
	ui::IImageStore* pStoreSvc = ui::GetImageStoresSvc();
	if ( pStoreSvc->RetrieveIcon( CIconId( dlgIconId, SmallIcon ) ) != nullptr )
		m_dlgIconId = dlgIconId;

	// main dialogs need to manage both small and large icons at once
	if ( pStoreSvc->RetrieveIcon( CIconId( dlgIconId, LargeIcon ) ) != nullptr )
		m_dlgIconId = dlgIconId;

	if ( m_dlgIconId != 0 )
		m_hideSysMenuIcon = false;
}

void CPopupDlgBase::SetupDlgIcons( void )
{
	// main dialogs need to manage both small and large icons at once
	CWnd* pWnd = dynamic_cast<CWnd*>( this );

	if ( const CIcon* pSmallIcon = GetDlgIcon( DlgSmallIcon ) )
		pWnd->SetIcon( pSmallIcon->GetHandle(), DlgSmallIcon );

	if ( const CIcon* pLargeIcon = GetDlgIcon( DlgLargeIcon ) )
		pWnd->SetIcon( pLargeIcon->GetHandle(), DlgLargeIcon );
}

bool CPopupDlgBase::CanAddAboutMenuItem( void ) const
{
	if ( m_noAboutMenuItem )
		return false;

	const CWnd* pWnd = dynamic_cast<const CWnd*>( this );
	ASSERT_PTR( pWnd->GetSafeHwnd() );
	return !is_a<CAboutBox>( pWnd ) && !is_a<CDialog>( pWnd->GetParent() );
}

void CPopupDlgBase::AddAboutMenuItem( CMenu* pMenu )
{
	ASSERT_PTR( pMenu );

	CVersionInfo version;
	std::tstring menuItemText = version.ExpandValues( _T("&About [InternalName]...\tF1") );

	pMenu->AppendMenu( MF_SEPARATOR );
	pMenu->AppendMenu( MF_STRING, ID_APP_ABOUT, menuItemText.c_str() );
	ui::SetMenuItemImage( *pMenu, ID_APP_ABOUT );
}

bool CPopupDlgBase::HandleCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_isTopDlg )
	{
		// Note: AfxGetThread() returns CURRENT module's CWinApp singleton, whether a DLL or EXE module.
		// The dialog may be hosted by a process with different architecture (e.g. Explorer.exe shell extension), with a different CWinApp singleton or none at all.

		if ( CWinThread* pCurrThread = AfxGetThread() )
			return pCurrThread->OnCmdMsg( id, code, pExtra, pHandlerInfo ) != FALSE;		// some commands may handled by the CWinApp
	}

	return false;
}


// CAccelPool implementation

CAccelPool::~CAccelPool( void )
{
	utl::ClearOwningContainer( m_accelTables );
}

void CAccelPool::AddAccelTable( CAccelTable* pAccelTable )
{
	utl::PushUnique( m_accelTables, pAccelTable );
}

bool CAccelPool::TranslateAccels( MSG* pMsg, HWND hDialog )
{
	ASSERT_PTR( hDialog );

	for ( std::vector<CAccelTable*>::const_iterator itAccelTable = m_accelTables.begin(); itAccelTable != m_accelTables.end(); ++itAccelTable )
		if ( ( *itAccelTable )->Translate( pMsg, hDialog ) )
			return true;

	return false;
}


// CPopupWndPool implementation

CPopupWndPool* CPopupWndPool::Instance( void )
{
	static CPopupWndPool s_popupPool;
	return &s_popupPool;
}

bool CPopupWndPool::AddWindow( CWnd* pPopupTopWnd )
{
	ASSERT_PTR( pPopupTopWnd );
	ASSERT( !utl::Contains( m_popupWnds, pPopupTopWnd ) );
	ASSERT( nullptr == pPopupTopWnd->GetSafeHwnd() || ui::IsTopLevel( pPopupTopWnd->GetSafeHwnd() ) );		// a top-level window?

	if ( pPopupTopWnd == AfxGetMainWnd() )
		return false;			// we keep track of all top-level popups except the main application window

	m_popupWnds.push_back( pPopupTopWnd );
	return true;
}

bool CPopupWndPool::RemoveWindow( CWnd* pPopupTopWnd )
{
	ASSERT_PTR( pPopupTopWnd );

	std::vector<CWnd*>::const_iterator itFound = std::find( m_popupWnds.begin(), m_popupWnds.end(), pPopupTopWnd );

	if ( itFound == m_popupWnds.end() )
		return false;

	m_popupWnds.erase( itFound );
	return true;
}

bool CPopupWndPool::SendIdleUpdates( CWnd* pPopupWnd )
{
	ASSERT_PTR( pPopupWnd );

	if ( pPopupWnd->GetSafeHwnd() != nullptr )
		if ( pPopupWnd->IsWindowVisible() )
		{
			// send WM_IDLEUPDATECMDUI to the main window
			AfxCallWndProc( pPopupWnd, pPopupWnd->m_hWnd, WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0 );
			pPopupWnd->SendMessageToDescendants( WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0, TRUE, TRUE );
			return true;
		}

	return false;
}

void CPopupWndPool::OnIdle( void )
{
	utl::for_each( m_popupWnds, CPopupWndPool::SendIdleUpdates );
}
