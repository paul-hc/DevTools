
#include "stdafx.h"
#include "LayoutDialog.h"
#include "LayoutEngine.h"
#include "CmdInfoStore.h"
#include "MenuUtilities.h"
#include "Utilities.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_initialSize[] = _T("DlgInitialSize");
	static const TCHAR entry_dialogPos[] = _T("DlgPosition");
	static const TCHAR entry_dialogSize[] = _T("DlgSize");
	static const TCHAR entry_showCmd[] = _T("DlgShowCmd");
	static const TCHAR entry_collapsed[] = _T("DlgCollapsed");

	static const TCHAR format_posSize[] = _T("%d, %d");
}


CLayoutDialog::CLayoutDialog( void )
	: MfcBaseDialog()
	, m_dlgFlags( Modeless )
{
	Construct();
}

CLayoutDialog::CLayoutDialog( UINT templateId, CWnd* pParent /*= NULL*/ )
	: MfcBaseDialog( templateId, pParent )
	, m_dlgFlags( 0 )
{
	Construct();
	m_initCentered = pParent != NULL;		// keep absolute position if main dialog
}

CLayoutDialog::CLayoutDialog( const TCHAR* pTemplateName, CWnd* pParent /*= NULL*/ )
	: MfcBaseDialog( pTemplateName, pParent )
	, m_dlgFlags( 0 )
{
	Construct();
	m_initCentered = pParent != NULL;		// keep absolute position if main dialog
}

CLayoutDialog::~CLayoutDialog()
{
}

void CLayoutDialog::Construct( void )
{
	m_initCollapsed = false;
	m_pLayoutEngine.reset( new CLayoutEngine );
}

CLayoutEngine& CLayoutDialog::GetLayoutEngine( void )
{
	return *m_pLayoutEngine;
}

void CLayoutDialog::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutDialog::HasControlLayout( void ) const
{
	return m_pLayoutEngine->HasCtrlLayout();
}

bool CLayoutDialog::UseWindowPlacement( void ) const
{
	DWORD style = GetStyle();
	if ( !m_initCentered && !HasFlag( style, WS_CHILD ) && !HasFlag( GetExStyle(), WS_EX_TOOLWINDOW ) )		// WS_EX_TOOLWINDOW uses screen coords (not workspace coords)
		return HasFlag( style, WS_MINIMIZEBOX | WS_MAXIMIZEBOX );
	return false;
}

void CLayoutDialog::SaveToRegistry( void )
{
	if ( m_regSection.empty() )
		return;
	if ( !m_pLayoutEngine->HasInitialSize() )
		return;			// not initialized

	CLayoutPlacement placement( UseWindowPlacement() );
	placement.m_initialSize = m_pLayoutEngine->GetMinClientSize( false );		// save original size to ignore saved layout for changed template (development changes)
	placement.m_collapsed = m_pLayoutEngine->IsCollapsed();

	WINDOWPLACEMENT wp = { sizeof( WINDOWPLACEMENT ) };
	if ( placement.m_useWindowPlacement && GetWindowPlacement( &wp ) )
	{
		CRect windowRect = wp.rcNormalPosition;					// workspace coordinates
		placement.m_showCmd = wp.showCmd;
		placement.m_pos = windowRect.TopLeft();
		placement.m_size = windowRect.Size();
	}
	else
	{
		CRect windowRect = ui::GetControlRect( m_hWnd );		// parent coords if child
		placement.m_pos = windowRect.TopLeft();
		placement.m_size = windowRect.Size();
	}

	CWinApp* pApp = AfxGetApp();
	std::tstring spec;

	spec = str::Format( reg::format_posSize, placement.m_initialSize.cx, placement.m_initialSize.cy );
	pApp->WriteProfileString( m_regSection.c_str(), reg::entry_initialSize, spec.c_str() );

	if ( !m_initCentered )
	{
		spec = str::Format( reg::format_posSize, placement.m_pos.x, placement.m_pos.y );
		pApp->WriteProfileString( m_regSection.c_str(), reg::entry_dialogPos, spec.c_str() );
	}

	spec = str::Format( reg::format_posSize, placement.m_size.cx, placement.m_size.cy );
	pApp->WriteProfileString( m_regSection.c_str(), reg::entry_dialogSize, spec.c_str() );

	if ( placement.m_useWindowPlacement )
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_showCmd, placement.m_showCmd );

	if ( m_pLayoutEngine->IsCollapsible() )
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_collapsed, placement.m_collapsed );
}

void CLayoutDialog::LoadFromRegistry( void )
{
	ASSERT_NULL( m_pPlacement.get() );

	if ( m_regSection.empty() )
		return;

	std::auto_ptr< CLayoutPlacement > pPlacement( new CLayoutPlacement( UseWindowPlacement() ) );
	CWinApp* pApp = AfxGetApp();
	std::tstring spec;

	spec = pApp->GetProfileString( m_regSection.c_str(), reg::entry_initialSize ).GetString();
	if ( _stscanf( spec.c_str(), reg::format_posSize, &pPlacement->m_initialSize.cx, &pPlacement->m_initialSize.cy ) != 2 )
		return;

	if ( !m_initCentered )
	{
		spec = pApp->GetProfileString( m_regSection.c_str(), reg::entry_dialogPos ).GetString();
		if ( _stscanf( spec.c_str(), reg::format_posSize, &pPlacement->m_pos.x, &pPlacement->m_pos.y ) != 2 )
			return;
	}

	spec = pApp->GetProfileString( m_regSection.c_str(), reg::entry_dialogSize ).GetString();
	if ( _stscanf( spec.c_str(), reg::format_posSize, &pPlacement->m_size.cx, &pPlacement->m_size.cy ) != 2 )
		return;

	if ( pPlacement->m_useWindowPlacement )
	{
		pPlacement->m_showCmd = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_showCmd, pPlacement->m_showCmd );
		if ( AfxGetMainWnd() == this )
			if ( pApp->m_nCmdShow != SW_SHOWNORMAL )
				pPlacement->m_showCmd = pApp->m_nCmdShow;		// override with the command line option
			else
				pApp->m_nCmdShow = pPlacement->m_showCmd;		// use the saved state
	}

	if ( m_pLayoutEngine->IsCollapsible() )
		pPlacement->m_collapsed = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_collapsed, pPlacement->m_collapsed ) != FALSE;

	if ( pPlacement->m_initialSize != m_pLayoutEngine->GetMinClientSize( false ) )		// template size changed during development
		return;

	m_pPlacement.reset( pPlacement.release() );
}

void CLayoutDialog::RestorePlacement( void )
{
	if ( NULL == m_pPlacement.get() )
		m_pLayoutEngine->SetCollapsed( m_initCollapsed );
	else
	{
		m_pLayoutEngine->SetCollapsed( m_pPlacement->m_collapsed );

		if ( m_pPlacement->m_useWindowPlacement )
		{
			CRect windowRect( m_pPlacement->m_pos, m_pPlacement->m_size );
			ui::EnsureVisibleDesktopRect( windowRect );

			WINDOWPLACEMENT wp = { sizeof( WINDOWPLACEMENT ), 0, m_pPlacement->m_showCmd };
			wp.ptMinPosition = CPoint( 0, 0 );
			wp.ptMaxPosition = CPoint( -1, -1 );
			wp.rcNormalPosition = windowRect;

			SetWindowPlacement( &wp );
		}
		else
		{
			CRect orgWindowRect;
			GetWindowRect( &orgWindowRect );

			CRect windowRect = orgWindowRect;

			if ( !m_initCentered )
				windowRect.OffsetRect( m_pPlacement->m_pos - windowRect.TopLeft() );

			ui::SetRectSize( windowRect, m_pPlacement->m_size );
			ui::EnsureVisibleDesktopRect( windowRect, !HasFlag( GetExStyle(), WS_EX_TOOLWINDOW ) ? ui::Workspace : ui::Monitor );

			if ( windowRect != orgWindowRect )
				MoveWindow( windowRect, IsWindowVisible() );
		}
	}

	if ( UseWindowPlacement() )
		PostRestorePlacement( m_pPlacement.get() != NULL ? m_pPlacement->m_showCmd : -1 );
	else if ( m_initCentered || NULL == m_pPlacement.get() )
		CenterWindow( GetOwner() );
}

// main dialog may override if it minimizes to system tray icon
void CLayoutDialog::PostRestorePlacement( int showCmd )
{
	showCmd;
}

void CLayoutDialog::ModifySystemMenu( void )
{
	if ( CMenu* pSystemMenu = GetSystemMenu( FALSE ) )
	{
		DWORD style = GetStyle();

		if ( !HasFlag( style, WS_MINIMIZEBOX | WS_MAXIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_RESTORE, MF_BYCOMMAND );
		if ( !HasFlag( style, WS_MINIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_MINIMIZE, MF_BYCOMMAND );
		if ( !HasFlag( style, WS_MAXIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_MAXIMIZE, MF_BYCOMMAND );

		// enable sizing if we haven't got a sizing dialog frame
		if ( m_resizable )
		{
			if ( UINT_MAX == pSystemMenu->GetMenuState( SC_SIZE, MF_BYCOMMAND ) )
			{
				pSystemMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_STRING, SC_SIZE, _T("&Size") );
				pSystemMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR );
			}

			pSystemMenu->EnableMenuItem( SC_SIZE, MF_BYCOMMAND | MF_ENABLED );
		}

		if ( CanAddAboutMenuItem() )
			AddAboutMenuItem( pSystemMenu );
	}

	if ( m_hideSysMenuIcon )
		if ( !HasFlag( GetStyle(), WS_CHILD ) && HasFlag( GetStyle(), WS_SYSMENU ) )
		{
			ModifyStyleEx( 0, WS_EX_DLGMODALFRAME );
			if ( NULL == GetIcon( ICON_SMALL ) )
				SetIcon( NULL, ICON_SMALL );			// remove default icon; this will not prevent adding an icon using SetIcon()
		}

	SetupDlgIcons();
}

void CLayoutDialog::LayoutDialog( void )
{
	if ( !IsIconic() )
		if ( m_pLayoutEngine->IsInitialized() )
			m_pLayoutEngine->LayoutControls();
}

void CLayoutDialog::OnIdleUpdateControls( void )
{
	SendMessageToDescendants( WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0, m_idleUpdateDeep, TRUE );			// update dialog toolbars
}

void CLayoutDialog::DoDataExchange( CDataExchange* pDX )
{
	if ( DialogOutput == pDX->m_bSaveAndValidate )
		if ( !m_pLayoutEngine->IsInitialized() )
		{
			m_pLayoutEngine->Initialize( this );

			if ( !HasFlag( GetStyle(), WS_CHILD ) )
			{
				if ( m_pLayoutEngine->HasCtrlLayout() )
					m_pLayoutEngine->CreateResizeGripper();

				EnableToolTips( TRUE );
			}

			LoadFromRegistry();
			RestorePlacement();
			ModifySystemMenu();
		}

	MfcBaseDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutDialog, MfcBaseDialog )
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_NCHITTEST()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_INITMENUPOPUP()
	ON_WM_SYSCOMMAND()
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, 0, 0xFFFF, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, 0, 0xFFFF, OnTtnNeedText )
	ON_MESSAGE( WM_KICKIDLE, OnKickIdle )
END_MESSAGE_MAP()

BOOL CLayoutDialog::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accelPool.TranslateAccels( pMsg, m_hWnd ) ||
		MfcBaseDialog::PreTranslateMessage( pMsg );
}

void CLayoutDialog::OnDestroy( void )
{
	SaveToRegistry();
	MfcBaseDialog::OnDestroy();
}

void CLayoutDialog::OnSize( UINT sizeType, int cx, int cy )
{
	MfcBaseDialog::OnSize( sizeType, cx, cy );

	if ( sizeType != SIZE_MINIMIZED )
		LayoutDialog();
}

void CLayoutDialog::OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo )
{
	MfcBaseDialog::OnGetMinMaxInfo( pMinMaxInfo );
	m_pLayoutEngine->HandleGetMinMaxInfo( pMinMaxInfo );
}

LRESULT CLayoutDialog::OnNcHitTest( CPoint screenPoint )
{
	return m_pLayoutEngine->HandleHitTest( MfcBaseDialog::OnNcHitTest( screenPoint ), screenPoint );
}

BOOL CLayoutDialog::OnEraseBkgnd( CDC* pDC )
{
	return
		m_pLayoutEngine->HandleEraseBkgnd( pDC ) ||
		MfcBaseDialog::OnEraseBkgnd( pDC );
}

void CLayoutDialog::OnPaint( void )
{
	MfcBaseDialog::OnPaint();
	m_pLayoutEngine->HandlePostPaint();
}

void CLayoutDialog::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	MfcBaseDialog::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

void CLayoutDialog::OnSysCommand( UINT cmdId, LPARAM lParam )
{
	if ( ID_APP_ABOUT == cmdId )
	{
		OnCommand( ID_APP_ABOUT, 0 );
		return;
	}
	MfcBaseDialog::OnSysCommand( cmdId, lParam );
}

BOOL CLayoutDialog::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// message handled

	return Default() != 0;
}

LRESULT CLayoutDialog::OnKickIdle( WPARAM, LPARAM )
{
	OnIdleUpdateControls();
	return Default();
}
