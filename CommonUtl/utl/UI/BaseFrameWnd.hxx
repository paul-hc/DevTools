#ifndef BaseFrameWnd_hxx
#define BaseFrameWnd_hxx

#include "BaseApp.h"
#include "SystemTray.h"
#include "WindowPlacement.h"
#include "resource.h"


// CBaseFrameWnd template code

template< typename BaseWndT >
CBaseFrameWnd<BaseWndT>::~CBaseFrameWnd()
{
}

template< typename BaseWndT >
CMenu* CBaseFrameWnd<BaseWndT>::GetTrayIconContextMenu( void ) override
{
	return UseSysTrayMinimize() ? &m_trayPopupMenu : nullptr;
}

template< typename BaseWndT >
bool CBaseFrameWnd<BaseWndT>::OnTrayIconNotify( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos ) override
{
	msgNotifyCode, trayIconId, screenPos;
	return false;
}

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::SaveWindowPlacement( void )
{
	if ( m_regSection.empty() )
		return;

	CWindowPlacement wp;

	if ( wp.ReadWnd( this ) )
		wp.RegSave( m_regSection.c_str() );
}

template< typename BaseWndT >
bool CBaseFrameWnd<BaseWndT>::LoadWindowPlacement( CREATESTRUCT* pCreateStruct )
{
	if ( !m_regSection.empty() )
	{
		CWindowPlacement wp;

		if ( wp.RegLoad( m_regSection.c_str(), this ) )
		{
			wp.QueryCreateStruct( pCreateStruct );
			m_restoreToMaximized = wp.IsRestoreToMaximized();
			return true;
		}
	}

	return false;
}

template< typename BaseWndT >
bool CBaseFrameWnd<BaseWndT>::ShowAppWindow( int cmdShow )
{
	if ( UseSysTrayMinimize() )
		if ( SW_SHOWMINIMIZED == cmdShow )
		{
			m_pSystemTray->MinimizeOwnerWnd( m_restoreToMaximized );
			return true;
		}

	return this->ShowWindow( cmdShow ) != FALSE;
}

template< typename BaseWndT >
BOOL CBaseFrameWnd<BaseWndT>::PreCreateWindow( CREATESTRUCT& cs ) override
{
	if ( !str::IsEmpty( cs.lpszName ) )		// called twice during frame creation: skip fist time when cs.lpszName is nullptr
	{
		const std::tstring& appNameSuffix = DYNAMIC_CAST_BASE_APP( GetAppNameSuffix() );

		if ( !appNameSuffix.empty() && -1 == m_strTitle.Find( appNameSuffix.c_str() ) )
		{
			m_strTitle += appNameSuffix.c_str();		// append ' [NN-bit]' suffix
			cs.lpszName = m_strTitle.GetString();
		}

		if ( PersistPlacement() )
			LoadWindowPlacement( &cs );
	}

	return __super::PreCreateWindow( cs );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseFrameWnd, BaseWndT, TBaseClass )
	ON_WM_CLOSE()
	ON_COMMAND( ID_APP_RESTORE, OnAppRestore )
	ON_UPDATE_COMMAND_UI( ID_APP_RESTORE, OnUpdateAppRestore )
	ON_COMMAND( ID_APP_MINIMIZE, OnAppMinimize )
	ON_UPDATE_COMMAND_UI( ID_APP_MINIMIZE, OnUpdateAppMinimize )
END_MESSAGE_MAP()

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::OnClose( void )
{
	if ( PersistPlacement() )
		SaveWindowPlacement();		// avoid saving placement on WM_DESTROY, since MFC hides the frame before destroying the window

	__super::OnClose();
}

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::OnAppRestore( void )
{
	this->SendMessage( WM_SYSCOMMAND, SC_RESTORE );
}

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::OnUpdateAppRestore( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CSystemTray::IsMinimizedToTray( this ) );
}

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::OnAppMinimize( void )
{
	this->SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
}

template< typename BaseWndT >
void CBaseFrameWnd<BaseWndT>::OnUpdateAppMinimize( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !CSystemTray::IsMinimizedToTray( AfxGetMainWnd() ) );
}


#endif // BaseFrameWnd_hxx
