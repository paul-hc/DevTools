#ifndef BaseFrameWnd_hxx
#define BaseFrameWnd_hxx

#include "SystemTray.h"
#include "WindowPlacement.h"


// CBaseFrameWnd template code

template< typename BaseWnd >
CBaseFrameWnd<BaseWnd>::~CBaseFrameWnd()
{
}

template< typename BaseWnd >
CMenu* CBaseFrameWnd<BaseWnd>::GetTrayIconContextMenu( void ) override
{
	return UseSysTrayMinimize() ? &m_trayPopupMenu : NULL;
}

template< typename BaseWnd >
bool CBaseFrameWnd<BaseWnd>::OnTrayIconNotify( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos ) override
{
	msgNotifyCode, trayIconId, screenPos;
	return false;
}

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::SaveWindowPlacement( void )
{
	if ( m_regSection.empty() )
		return;

	CWindowPlacement wp;

	if ( wp.ReadWnd( this ) )
		wp.RegSave( m_regSection.c_str() );
}

template< typename BaseWnd >
bool CBaseFrameWnd<BaseWnd>::LoadWindowPlacement( CREATESTRUCT* pCreateStruct )
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

template< typename BaseWnd >
bool CBaseFrameWnd<BaseWnd>::ShowAppWindow( int cmdShow )
{
	if ( UseSysTrayMinimize() )
		if ( SW_SHOWMINIMIZED == cmdShow )
		{
			m_pSystemTray->MinimizeOwnerWnd( m_restoreToMaximized );
			return true;
		}

	return ShowWindow( cmdShow ) != FALSE;
}

template< typename BaseWnd >
BOOL CBaseFrameWnd<BaseWnd>::PreCreateWindow( CREATESTRUCT& rCreateStruct ) override
{
	if ( !str::IsEmpty( rCreateStruct.lpszClass ) )		// called twice during frame creation: skip fist time when lpszClass is NULL
		if ( PersistPlacement() )
			LoadWindowPlacement( &rCreateStruct );

	return __super::PreCreateWindow( rCreateStruct );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseFrameWnd, BaseWnd, TBaseClass )
	ON_WM_CLOSE()
	ON_COMMAND( ID_APP_RESTORE, OnAppRestore )
	ON_UPDATE_COMMAND_UI( ID_APP_RESTORE, OnUpdateAppRestore )
	ON_COMMAND( ID_APP_MINIMIZE, OnAppMinimize )
	ON_UPDATE_COMMAND_UI( ID_APP_MINIMIZE, OnUpdateAppMinimize )
END_MESSAGE_MAP()

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::OnClose( void )
{
	if ( PersistPlacement() )
		SaveWindowPlacement();		// avoid saving placement on WM_DESTROY, since MFC hides the frame before destroying the window

	__super::OnClose();
}

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::OnAppRestore( void )
{
	SendMessage( WM_SYSCOMMAND, SC_RESTORE );
}

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::OnUpdateAppRestore( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CSystemTray::IsMinimizedToTray( this ) );
}

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::OnAppMinimize( void )
{
	SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
}

template< typename BaseWnd >
void CBaseFrameWnd<BaseWnd>::OnUpdateAppMinimize( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !CSystemTray::IsMinimizedToTray( AfxGetMainWnd() ) );
}


#endif // BaseFrameWnd_hxx
