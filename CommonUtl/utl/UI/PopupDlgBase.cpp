
#include "stdafx.h"
#include "PopupDlgBase.h"
#include "AboutBox.h"
#include "AccelTable.h"
#include "ContainerUtilities.h"
#include "ImageStore.h"
#include "MenuUtilities.h"
#include "VersionInfo.h"

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
	return NULL;
}

void CPopupDlgBase::LoadDlgIcon( UINT dlgIconId )
{
	// normally called prior to creation
	ui::IImageStore* pStoreSvc = ui::GetImageStoresSvc();
	if ( pStoreSvc->RetrieveIcon( CIconId( dlgIconId, SmallIcon ) ) != NULL )
		m_dlgIconId = dlgIconId;

	// main dialogs need to manage both small and large icons at once
	if ( pStoreSvc->RetrieveIcon( CIconId( dlgIconId, LargeIcon ) ) != NULL )
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

	for ( std::vector< CAccelTable* >::const_iterator itAccelTable = m_accelTables.begin(); itAccelTable != m_accelTables.end(); ++itAccelTable )
		if ( ( *itAccelTable )->Translate( pMsg, hDialog ) )
			return true;

	return false;
}
