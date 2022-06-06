
#include "stdafx.h"
#include "LayoutPropertySheet.h"
#include "LayoutPropertyPage.h"
#include "LayoutEngine.h"
#include "CmdUpdate.h"
#include "WndUtils.h"
#include "StringUtilities.h"
#include "BaseApp.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_initialSize[] = _T("InitialSize");
	static const TCHAR entry_sheetPos[] = _T("SheetPos");
	static const TCHAR entry_sheetSize[] = _T("SheetSize");

	static const TCHAR format_posSize[] = _T("%d, %d");
}


static CLayoutStyle layoutStyles[] =
{
	{ AFX_IDC_TAB_CONTROL, layout::Size },
	{ IDOK, layout::Move },
	{ IDCANCEL, layout::Move },
	{ ID_APPLY_NOW, layout::Move },
	{ IDHELP, layout::Move }
};

CLayoutPropertySheet::CLayoutPropertySheet( const std::tstring& title, CWnd* pParent, UINT selPageIndex /*= 0*/ )
	: CLayoutBasePropertySheet( title.c_str(), pParent, selPageIndex )
	, m_pLayoutEngine( new CLayoutEngine( CLayoutEngine::Normal ) )		// no groups used -> no smoothing required
	, m_restorePos( PosNoRestore )
	, m_restoreSize( true )
	, m_singleTransactionButtons( ShowOkCancel )
	, m_styleMinMax( 0 )
	, m_alwaysModified( false )
{
	m_psh.dwFlags &= ~PSH_HASHELP;				// don't show the help button by default
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, COUNT_OF( layoutStyles ) );
	m_resizable = false;						// auto: will be flipped to true if any page is resizable
	m_initCentered = false;
}

CLayoutPropertySheet::~CLayoutPropertySheet()
{
}

bool CLayoutPropertySheet::CreateModeless( CWnd* pParent /*= NULL*/, DWORD style /*= UINT_MAX*/, DWORD styleEx /*= 0*/ )
{
	if ( UINT_MAX == style )
	{
		style = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | DS_3DLOOK | DS_CONTEXTHELP | DS_SETFONT;
		if ( m_resizable )
			style |= WS_THICKFRAME;
		else
			style |= DS_MODALFRAME;
	}

	m_modeless = m_autoDelete = true;
	m_isTopDlg = true;
	ENSURE( !HasFlag( style, WS_CHILD ) );		// for child property sheets must use CLayoutChildPropertySheet base class

	return __super::Create( pParent, style, styleEx ) != FALSE;
}

CLayoutEngine& CLayoutPropertySheet::GetLayoutEngine( void )
{
	return *m_pLayoutEngine;
}

void CLayoutPropertySheet::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutPropertySheet::HasControlLayout( void ) const
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutPropertySheet::BuildPropPageArray( void )
{
	if ( !m_resizable )
		m_resizable = AnyPageResizable();

	__super::BuildPropPageArray();
}

void CLayoutPropertySheet::LoadFromRegistry( void )
{
	__super::LoadFromRegistry();

	ASSERT_NULL( m_pSheetPlacement.get() );

	if ( m_regSection.empty() || !m_resizable )
		return;

	CLayoutPlacement placement( false );
	std::tstring spec;

	spec = AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_initialSize ).GetString();
	if ( _stscanf( spec.c_str(), reg::format_posSize, &placement.m_initialSize.cx, &placement.m_initialSize.cy ) != 2 )
		return;

	if ( PosRestore == m_restorePos )
	{
		spec = AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_sheetPos ).GetString();
		if ( _stscanf( spec.c_str(), reg::format_posSize, &placement.m_pos.x, &placement.m_pos.y ) != 2 )
			return;
	}

	if ( m_restoreSize )
	{
		spec = AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_sheetSize ).GetString();
		if ( _stscanf( spec.c_str(), reg::format_posSize, &placement.m_size.cx, &placement.m_size.cy ) != 2 )
			return;
	}

	m_pSheetPlacement.reset( new CLayoutPlacement( placement ) );
}

void CLayoutPropertySheet::SaveToRegistry( void )
{
	__super::SaveToRegistry();

	if ( m_regSection.empty() || !m_resizable )
		return;

	CRect sheetRect;
	GetWindowRect( &sheetRect );

	CLayoutPlacement placement( false );
	placement.m_initialSize = m_pLayoutEngine->GetMinClientSize();		// save original size to ignore saved layout for changed page templates (development changes)
	placement.m_pos = sheetRect.TopLeft();
	placement.m_size = sheetRect.Size();

	if ( 0 == placement.m_initialSize.cx || 0 == placement.m_initialSize.cy )
		return;				// not initialized

	std::tstring spec;

	spec = str::Format( reg::format_posSize, placement.m_initialSize.cx, placement.m_initialSize.cy );
	AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_initialSize, spec.c_str() );

	if ( PosRestore == m_restorePos )
	{
		spec = str::Format( reg::format_posSize, placement.m_pos.x, placement.m_pos.y );
		AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_sheetPos, spec.c_str() );
	}

	if ( m_restoreSize )
	{
		spec = str::Format( reg::format_posSize, placement.m_size.cx, placement.m_size.cy );
		AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_sheetSize, spec.c_str() );
	}
}

void CLayoutPropertySheet::SetupInitialSheet( void )
{
	if ( IsSingleTransaction() && ShowOnlyClose == m_singleTransactionButtons )
	{
		ui::MoveControlOver( *this, IDOK, IDCANCEL );
		SendMessage( PSM_CANCELTOCLOSE );
	}
	else
	{	// show existing standard buttons
		ui::ShowControl( *this, IDOK );
		ui::EnableControl( *this, IDOK );

		ui::ShowControl( *this, IDCANCEL );
		ui::EnableControl( *this, IDCANCEL );

		if ( CWnd* pApplyButton = GetDlgItem( ID_APPLY_NOW ) )
			ui::ShowWindow( *pApplyButton );

		if ( HasHelpButton() )
		{
			ui::ShowControl( *this, IDHELP );
			ui::EnableControl( *this, IDHELP );
		}
	}

	if ( IsModeless() )
		AdjustModelessSheet();

	if ( m_alwaysModified )
		SetSheetModified( true );
}

void CLayoutPropertySheet::AdjustModelessSheet( void )
{
	ASSERT( IsModeless() );

	// modeless sheet gets shrunk vertically, so the bottom buttons aren't visible;
	// reposition property sheet so that button area is accounted for
	enum { ButtonSpacingV = 8 };
	CRect sheetRect, sheetClientRect;
	GetWindowRect( &sheetRect );
	GetClientRect( &sheetClientRect ); ClientToScreen( &sheetClientRect );
	int sheetEdgeV = sheetRect.bottom - sheetClientRect.bottom;

	CRect okButtonRect;
	if ( CWnd* pOkButton = GetDlgItem( IDOK ) )
		pOkButton->GetWindowRect( &okButtonRect );

	sheetRect.bottom = okButtonRect.bottom + ButtonSpacingV + sheetEdgeV;

	SetWindowPos( NULL, sheetRect.left, sheetRect.top, sheetRect.Width(), sheetRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE );
}

void CLayoutPropertySheet::ModifySystemMenu( void )
{
	// validate sizing if we haven't a sizing dialog frame
	if ( CMenu* pSystemMenu = GetSystemMenu( FALSE ) )
	{
		if ( !HasFlag( m_styleMinMax, WS_MINIMIZEBOX | WS_MAXIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_RESTORE, MF_BYCOMMAND );
		if ( !HasFlag( m_styleMinMax, WS_MINIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_MINIMIZE, MF_BYCOMMAND );
		if ( !HasFlag( m_styleMinMax, WS_MAXIMIZEBOX ) )
			pSystemMenu->DeleteMenu( SC_MAXIMIZE, MF_BYCOMMAND );

		if ( UINT_MAX == pSystemMenu->GetMenuState( SC_SIZE, MF_BYCOMMAND ) )
		{
			pSystemMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_STRING, SC_SIZE, _T("&Size") );
			pSystemMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR );
		}

		pSystemMenu->EnableMenuItem( SC_SIZE, MF_BYCOMMAND | MF_ENABLED );

		if ( CanAddAboutMenuItem() )
			AddAboutMenuItem( pSystemMenu );
	}

	if ( m_resizable )
		if ( m_hideSysMenuIcon && HasFlag( GetStyle(), WS_SYSMENU ) )
		{
			ModifyStyleEx( 0, WS_EX_DLGMODALFRAME );
			if ( NULL == GetIcon( FALSE ) )
				SetIcon( NULL, FALSE );						// remove default icon; this will not prevent adding an icon using SetIcon()
		}

	SetupDlgIcons();
}

void CLayoutPropertySheet::RestoreSheetPlacement( void )
{
	if ( m_pSheetPlacement.get() != NULL )
		if ( m_pSheetPlacement->m_initialSize == m_pLayoutEngine->GetMinClientSize() )		// hasn't changed during development?
		{
			CRect orgSheetRect;
			GetWindowRect( &orgSheetRect );

			CRect sheetRect = orgSheetRect;

			if ( PosRestore == m_restorePos )
				sheetRect.TopLeft() = m_pSheetPlacement->m_pos - sheetRect.TopLeft();

			if ( m_restoreSize )
				sheetRect.BottomRight() = sheetRect.TopLeft() + m_pSheetPlacement->m_size;

			if ( !m_initCentered )
			{
				CRect desktopRect;
				CWnd::GetDesktopWindow()->GetWindowRect( &desktopRect );
				ui::EnsureVisibleRect( sheetRect, desktopRect );
			}

			if ( sheetRect != orgSheetRect )
				MoveWindow( sheetRect, IsWindowVisible() );
		}

	if ( m_initCentered )
		CenterWindow( GetOwner() );
}

CPoint CLayoutPropertySheet::GetCascadeByOffset( void ) const
{
	CRect windowRect, clientRect;
	GetWindowRect( &windowRect );
	GetClientRect( &clientRect );
	ClientToScreen( &clientRect );

	int moveBy = clientRect.top - windowRect.top;
	return CPoint( moveBy, moveBy );
}

void CLayoutPropertySheet::OnIdleUpdateControls( void )
{
}

bool CLayoutPropertySheet::IsSheetModified( void ) const
{
	if ( NULL == m_hWnd )
		return false;				// window not yet created

	return m_alwaysModified || __super::IsSheetModified();
}

void CLayoutPropertySheet::LayoutSheet( void )
{
	if ( IsIconic() )
		return;

	if ( m_pLayoutEngine->IsInitialized() )
		m_pLayoutEngine->LayoutControls();

	__super::LayoutSheet();
}

void CLayoutPropertySheet::PreSubclassWindow( void )
{
	CLayoutBasePropertySheet::PreSubclassWindow();

	if ( IsModeless() )
	{
		m_modeless = m_autoDelete = true;
		CPopupWndPool::Instance()->AddWindow( this );
	}
}

void CLayoutPropertySheet::PostNcDestroy( void )
{
	CLayoutBasePropertySheet::PostNcDestroy();

	if ( m_autoDelete )
		delete this;
}

BOOL CLayoutPropertySheet::PreTranslateMessage( MSG* pMsg )
{
	return
		__super::PreTranslateMessage( pMsg ) ||				// handle base first because it must relay tooltip events
		m_accelPool.TranslateAccels( pMsg, m_hWnd );
}

BOOL CLayoutPropertySheet::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		HandleCmdMsg( id, code, pExtra, pHandlerInfo );							// some commands may handled by the CWinApp
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutPropertySheet, CLayoutBasePropertySheet )
	ON_WM_NCCREATE()
	ON_WM_GETMINMAXINFO()
	ON_WM_CONTEXTMENU()
	ON_WM_INITMENUPOPUP()
	ON_WM_NCHITTEST()
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
	ON_COMMAND( ID_APPLY_NOW, OnApplyNow )
	ON_COMMAND( IDOK, OnOk )
	ON_COMMAND( IDCANCEL, OnCancel )
	ON_MESSAGE( WM_KICKIDLE, OnKickIdle )
END_MESSAGE_MAP()

void CLayoutPropertySheet::OnDestroy( void )
{
	if ( m_modeless )
		CPopupWndPool::Instance()->RemoveWindow( this );

	CLayoutBasePropertySheet::OnDestroy();
}

BOOL CLayoutPropertySheet::OnInitDialog( void )
{
	BOOL result = __super::OnInitDialog();

	if ( m_styleMinMax != 0 )
		ModifyStyle( 0, m_styleMinMax, SWP_FRAMECHANGED );

	SetupInitialSheet();			// default sheet initialization: prior to restoring position/size
	ASSERT( !m_pLayoutEngine->IsInitialized() );
	m_pLayoutEngine->Initialize( this );

	if ( m_resizable && m_pLayoutEngine->HasCtrlLayout() )
		m_pLayoutEngine->CreateResizeGripper( CSize( 2, 1 ) );		// push it further out of the way of Apply button

	RestoreSheetPlacement();
	ModifySystemMenu();
	return result;
}

BOOL CLayoutPropertySheet::OnNcCreate( CREATESTRUCT* pCreate )
{
	if ( m_resizable )
		if ( !HasFlag( pCreate->style, WS_THICKFRAME ) )
		{
			ModifyStyle( DS_MODALFRAME, WS_THICKFRAME );			// make it a resizable sheet

			// the sheet gets slightly shrinked by the new resizable border, so resize it to keep client area the same size
			int deltaFrame = GetSystemMetrics( SM_CXSIZEFRAME ) - GetSystemMetrics( SM_CXDLGFRAME );
			CRect windowRect;
			GetWindowRect( &windowRect );

			windowRect.InflateRect( deltaFrame, deltaFrame );
			MoveWindow( &windowRect, false );
		}

	return __super::OnNcCreate( pCreate );
}

void CLayoutPropertySheet::OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo )
{
	__super::OnGetMinMaxInfo( pMinMaxInfo );

	CPoint minWindowSize = m_pLayoutEngine->GetMinWindowSize();
	pMinMaxInfo->ptMinTrackSize = minWindowSize;
}

void CLayoutPropertySheet::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
		app::TrackUnitTestMenu( this, screenPos );
	else
		__super::OnContextMenu( pWnd, screenPos );
}

void CLayoutPropertySheet::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

LRESULT CLayoutPropertySheet::OnNcHitTest( CPoint point )
{
	LRESULT hitTest = __super::OnNcHitTest( point );

	if ( HTCLIENT == hitTest )
		if ( layout::CResizeGripper* pResizeGripper = m_pLayoutEngine->GetResizeGripper() )
		{
			CPoint clientPoint = point;
			ScreenToClient( &clientPoint );

			if ( pResizeGripper->GetRect().PtInRect( clientPoint ) )
				return HTBOTTOMRIGHT;
		}

	return hitTest;
}

void CLayoutPropertySheet::OnPaint( void )
{
	__super::OnPaint();
	m_pLayoutEngine->HandlePostPaint();
}

void CLayoutPropertySheet::OnSysCommand( UINT cmdId, LPARAM lParam )
{
	if ( ID_APP_ABOUT == cmdId )
	{
		OnCommand( ID_APP_ABOUT, 0 );
		return;
	}
	__super::OnSysCommand( cmdId, lParam );
}

void CLayoutPropertySheet::OnApplyNow( void )
{
	ApplyChanges();
}

void CLayoutPropertySheet::OnOk( void )
{
	if ( ApplyChanges() )
		if ( m_bModeless )
			DestroyWindow();
		else
			EndDialog( IDOK );
}

void CLayoutPropertySheet::OnCancel( void )
{
	if ( m_bModeless )
		DestroyWindow();
	else
		EndDialog( IDCANCEL );
}

LRESULT CLayoutPropertySheet::OnKickIdle( WPARAM wParam, LPARAM lParam )
{
	OnIdleUpdateControls();
	return __super::OnKickIdle( wParam, lParam );
}
