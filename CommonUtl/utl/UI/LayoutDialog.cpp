
#include "pch.h"
#include "LayoutDialog.h"
#include "LayoutEngine.h"
#include "CmdUpdate.h"
#include "CmdInfoStore.h"
#include "WindowPlacement.h"
#include "WndUtils.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_initialSize[] = _T("DlgInitialSize");
	static const TCHAR entry_dialogPos[] = _T("DlgPosition");
	static const TCHAR entry_dialogSize[] = _T("DlgSize");
	static const TCHAR entry_restoreToMax[] = _T("DlgRestoreToMax");
	static const TCHAR entry_showCmd[] = _T("DlgShowCmd");
	static const TCHAR entry_collapsed[] = _T("DlgCollapsed");

	static const TCHAR format_posSize[] = _T("%d, %d");
}


CLayoutDialog::CLayoutDialog( void )
	: TMfcBaseDialog()
{
	Construct();

	m_modeless = m_autoDelete = true;		// default constructor is for modeless dialog
}

CLayoutDialog::CLayoutDialog( UINT templateId, CWnd* pParent /*= nullptr*/ )
	: TMfcBaseDialog( templateId, pParent )
{
	Construct();
	m_initCentered = pParent != nullptr;		// keep absolute position if main dialog
}

CLayoutDialog::CLayoutDialog( const TCHAR* pTemplateName, CWnd* pParent /*= nullptr*/ )
	: TMfcBaseDialog( pTemplateName, pParent )
{
	Construct();
	m_initCentered = pParent != nullptr;		// keep absolute position if main dialog
}

CLayoutDialog::~CLayoutDialog()
{
}

void CLayoutDialog::Construct( void )
{
	m_initCollapsed = false;
	m_pLayoutEngine.reset( new CLayoutEngine() );
}

bool CLayoutDialog::CreateModeless( UINT templateId /*= 0*/, CWnd* pParentWnd /*= nullptr*/, int cmdShow /*= SW_SHOW*/ )
{
	// works with both modal and modeless constructors:
	//	pass templateId and pParentWnd if not initialized via modal constructor
	//	if pParentWnd is NULL it uses the main wondow as parent
	//
	m_modeless = m_autoDelete = true;

	if ( !__super::Create( templateId != 0 ? MAKEINTRESOURCE( templateId ) : m_lpszTemplateName, pParentWnd != nullptr ? pParentWnd : m_pParentWnd ) )
		return false;

	if ( cmdShow != SW_HIDE )
		ShowWindow( cmdShow );

	return true;
}

CLayoutEngine& CLayoutDialog::GetLayoutEngine( void ) override
{
	return *m_pLayoutEngine;
}

void CLayoutDialog::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) override
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutDialog::HasControlLayout( void ) const override
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	rText, cmdId, pTooltip;
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

	CWindowPlacement wp;
	if ( placement.m_useWindowPlacement && wp.ReadWnd( this ) )
	{
		CRect windowRect = wp.rcNormalPosition;					// workspace coordinates
		placement.m_restoreToMaximized = wp.IsRestoreToMaximized();
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
	{
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_showCmd, placement.m_showCmd );
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_restoreToMax, placement.m_restoreToMaximized );
	}

	if ( m_pLayoutEngine->IsCollapsible() )
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_collapsed, placement.m_collapsed );
}

void CLayoutDialog::LoadFromRegistry( void )
{
	ASSERT_NULL( m_pPlacement.get() );

	if ( m_regSection.empty() )
		return;

	std::auto_ptr<CLayoutPlacement> pPlacement( new CLayoutPlacement( UseWindowPlacement() ) );
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
		pPlacement->m_restoreToMaximized = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_restoreToMax, pPlacement->m_restoreToMaximized ) != FALSE;

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
	if ( nullptr == m_pPlacement.get() )
		m_pLayoutEngine->SetCollapsed( m_initCollapsed );
	else
	{
		m_pLayoutEngine->SetCollapsed( m_pPlacement->m_collapsed );

		if ( m_pPlacement->m_useWindowPlacement )
		{
			CRect windowRect( m_pPlacement->m_pos, m_pPlacement->m_size );
			CWindowPlacement wp;

			wp.Setup( this, windowRect, m_pPlacement->m_showCmd );
			wp.CommitWnd( this );
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
		PostRestorePlacement( m_pPlacement.get() != nullptr ? m_pPlacement->m_showCmd : -1, m_pPlacement.get() != nullptr && m_pPlacement->m_restoreToMaximized );
	else if ( m_initCentered || nullptr == m_pPlacement.get() )
		CenterWindow( GetOwner() );
}

// main dialog may override if it minimizes to system tray icon
void CLayoutDialog::PostRestorePlacement( int showCmd, bool restoreToMaximized )
{
	showCmd, restoreToMaximized;
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
			if ( nullptr == GetIcon( ICON_SMALL ) )
				SetIcon( nullptr, ICON_SMALL );			// remove default icon; this will not prevent adding an icon using SetIcon()
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

void CLayoutDialog::PreSubclassWindow( void ) override
{
	TMfcBaseDialog::PreSubclassWindow();

	if ( m_modeless && ui::IsTopLevel( m_hWnd ) )
		CPopupWndPool::Instance()->AddWindow( this );
}

void CLayoutDialog::PostNcDestroy( void ) override
{
	TMfcBaseDialog::PostNcDestroy();

	if ( m_autoDelete )
		delete this;
}

void CLayoutDialog::DoDataExchange( CDataExchange* pDX ) override
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

	__super::DoDataExchange( pDX );
}

BOOL CLayoutDialog::DestroyWindow( void ) override
{
	// fix for app losing activation when destroying the modeless dialog: https://stackoverflow.com/questions/3144004/wpf-app-loses-completely-focus-on-window-close
	if ( IsModeless() && m_hWnd != nullptr )
		if ( CWnd* pOwner = GetOwner() )
			pOwner->SetActiveWindow();

	return __super::DestroyWindow();
}

BOOL CLayoutDialog::PreTranslateMessage( MSG* pMsg ) override
{
	return
		m_accelPool.TranslateAccels( pMsg, m_hWnd ) ||
		__super::PreTranslateMessage( pMsg );
}

BOOL CLayoutDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		HandleCmdMsg( id, code, pExtra, pHandlerInfo );							// some commands may handled by the CWinApp
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutDialog, TMfcBaseDialog )
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_NCHITTEST()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_INITMENUPOPUP()
	ON_WM_SYSCOMMAND()
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_MESSAGE( WM_KICKIDLE, OnKickIdle )
END_MESSAGE_MAP()

void CLayoutDialog::OnDestroy( void )
{
	SaveToRegistry();

	if ( m_modeless && ui::IsTopLevel( m_hWnd ) )
		CPopupWndPool::Instance()->RemoveWindow( this );

	TMfcBaseDialog::OnDestroy();
}

void CLayoutDialog::OnOK( void ) override
{
	if ( IsModeless() )
	{
		if ( UpdateData( DialogSaveChanges ) )
			DestroyWindow();
	}
	else
		__super::OnOK();
}

void CLayoutDialog::OnCancel( void ) override
{
	if ( IsModeless() )
		DestroyWindow();
	else
		__super::OnCancel();
}

void CLayoutDialog::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( sizeType != SIZE_MINIMIZED )
		LayoutDialog();
}

void CLayoutDialog::OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo )
{
	__super::OnGetMinMaxInfo( pMinMaxInfo );
	m_pLayoutEngine->HandleGetMinMaxInfo( pMinMaxInfo );
}

LRESULT CLayoutDialog::OnNcHitTest( CPoint screenPoint )
{
	return m_pLayoutEngine->HandleHitTest( __super::OnNcHitTest( screenPoint ), screenPoint );
}

BOOL CLayoutDialog::OnEraseBkgnd( CDC* pDC )
{
	return
		m_pLayoutEngine->HandleEraseBkgnd( pDC ) ||
		__super::OnEraseBkgnd( pDC );
}

void CLayoutDialog::OnPaint( void )
{
	__super::OnPaint();
	m_pLayoutEngine->HandlePostPaint();
}

void CLayoutDialog::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

void CLayoutDialog::OnSysCommand( UINT cmdId, LPARAM lParam )
{
	if ( ID_APP_ABOUT == cmdId )
	{
		OnCommand( ID_APP_ABOUT, 0 );
		return;
	}
	__super::OnSysCommand( cmdId, lParam );
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
