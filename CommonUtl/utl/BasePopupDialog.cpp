
#include "stdafx.h"
#include "BasePopupDialog.h"
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


// CBasePopupDialog implementation

CBasePopupDialog::CBasePopupDialog( void )
	: m_resizable( true )
	, m_initCentered( true )
	, m_hideSysMenuIcon( true )
	, m_noAboutMenuItem( false )
	, m_dlgIconId( 0 )
{
	m_accelPool.AddAccelTable( new CAccelTable( keys, COUNT_OF( keys ) ) );
}

const CIcon* CBasePopupDialog::GetDlgIcon( DlgIcon dlgIcon /*= DlgSmallIcon*/ ) const
{
	if ( m_dlgIconId != 0 && CImageStore::HasSharedStore() )
		return CImageStore::GetSharedStore()->RetrieveIcon( CIconId( m_dlgIconId, DlgSmallIcon == dlgIcon ? SmallIcon : LargeIcon ) );
	return NULL;
}

void CBasePopupDialog::LoadDlgIcon( UINT dlgIconId )
{
	// normally called prior to creation
	CImageStore* pSharedStore = CImageStore::SharedStore();
	if ( pSharedStore->RetrieveIcon( CIconId( dlgIconId, SmallIcon ) ) != NULL )
		m_dlgIconId = dlgIconId;

	// main dialogs need to manage both small and large icons at once
	if ( pSharedStore->RetrieveIcon( CIconId( dlgIconId, LargeIcon ) ) != NULL )
		m_dlgIconId = dlgIconId;

	if ( m_dlgIconId != 0 )
		m_hideSysMenuIcon = false;
}

void CBasePopupDialog::SetupDlgIcons( void )
{
	// main dialogs need to manage both small and large icons at once
	CWnd* pWnd = dynamic_cast< CWnd* >( this );

	if ( const CIcon* pSmallIcon = GetDlgIcon( DlgSmallIcon ) )
		pWnd->SetIcon( pSmallIcon->GetHandle(), DlgSmallIcon );

	if ( const CIcon* pLargeIcon = GetDlgIcon( DlgLargeIcon ) )
		pWnd->SetIcon( pLargeIcon->GetHandle(), DlgLargeIcon );
}

bool CBasePopupDialog::CanAddAboutMenuItem( void ) const
{
	if ( m_noAboutMenuItem )
		return false;

	const CWnd* pWnd = dynamic_cast< const CWnd* >( this );
	ASSERT_PTR( pWnd->GetSafeHwnd() );
	return !is_a< CAboutBox >( pWnd ) && !is_a< CDialog >( pWnd->GetParent() );
}

void CBasePopupDialog::AddAboutMenuItem( CMenu* pMenu )
{
	ASSERT_PTR( pMenu );

	CVersionInfo version;
	std::tstring menuItemText = version.ExpandValues( _T("&About [InternalName]...\tF1") );

	pMenu->AppendMenu( MF_SEPARATOR );
	pMenu->AppendMenu( MF_STRING, ID_APP_ABOUT, menuItemText.c_str() );
	ui::SetMenuItemImage( *pMenu, ID_APP_ABOUT );
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
