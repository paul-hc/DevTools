
#include "pch.h"
#include "MainDialog.h"
#include "WndHighlighter.h"
#include "Application.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_FOUND_WINDOW_INFO_EDIT, Size }
	};
}

CMainDialog::CMainDialog( void )
	: CBaseMainDialog( IDD_MAIN_DIALOG )
{
	m_regSection = _T("MainDialog");
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cx = 0;

	m_accelPool.AddAccelTable( new CAccelTable( IDD_MAIN_DIALOG ) );

	m_wndPickerTool.SetTrackIconId( CIconId( IDI_TRACKING_ICON, LargeIcon ) );
	m_wndPickerTool.LoadTrackCursor( IDR_TRACKING_CURSOR );
}

CMainDialog::~CMainDialog()
{
}

void CMainDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_wndPickerTool.m_hWnd;
	COptions* pOptions = app::GetOptions();

	DDX_Control( pDX, IDC_TRACK_TOOL_ICON, m_wndPickerTool );
	DDX_Control( pDX, IDC_FRAME_STYLE_COMBO, m_frameStyleCombo );
	DDX_Control( pDX, IDC_FRAME_SIZE_SPIN, m_frameSizeSpin );

	if ( firstInit )
	{
		m_frameSizeSpin.SetRange32( 1, 1500 );
		SetDlgItemText( IDC_TRACKING_POS_STATIC, _T("") );
	}

	ui::DDX_Bool( pDX, IDC_IGNORE_DISABLED_CHECK, pOptions->m_ignoreDisabled );
	ui::DDX_Bool( pDX, IDC_IGNORE_HIDDEN_CHECK, pOptions->m_ignoreHidden );
	ui::DDX_Bool( pDX, IDC_CACHE_DESTOP_DC_CHECK, CWndHighlighter::m_cacheDesktopDC );
	ui::DDX_Bool( pDX, IDC_REDRAW_AT_END_CHECK, CWndHighlighter::m_redrawAtEnd );
	DDX_CBIndex( pDX, IDC_FRAME_STYLE_COMBO, (int&)pOptions->m_frameStyle );
	DDX_Text( pDX, IDC_FRAME_SIZE_EDIT, pOptions->m_frameSize );

	if ( pDX->m_bSaveAndValidate )
		pOptions->Save();			// save right away so that new app instances read the current state

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainDialog, CBaseMainDialog )
	ON_TSN_TRACK( IDC_TRACK_TOOL_ICON, OnTfnTrackMoved )
	ON_TSWN_FOUNDWINDOW( IDC_TRACK_TOOL_ICON, OnTfnTrackFoundWindow )
	ON_COMMAND( ID_HIGHLIGHT_WINDOW, OnHighlight )
	ON_REGISTERED_MESSAGE( CTimerSequenceHook::WM_ENDTIMERSEQ, OnEndHighlight )
	ON_BN_CLICKED( IDC_IGNORE_HIDDEN_CHECK, OnOptionChanged )
	ON_BN_CLICKED( IDC_IGNORE_DISABLED_CHECK, OnOptionChanged )
	ON_BN_CLICKED( IDC_CACHE_DESTOP_DC_CHECK, OnOptionChanged )
	ON_BN_CLICKED( IDC_REDRAW_AT_END_CHECK, OnOptionChanged )
	ON_CBN_SELCHANGE( IDC_FRAME_STYLE_COMBO, OnOptionChanged )
	ON_EN_CHANGE( IDC_FRAME_SIZE_EDIT, OnOptionChanged )
END_MESSAGE_MAP()

void CMainDialog::OnTfnTrackMoved( void )
{
	CPoint cursorPos = m_wndPickerTool.GetCursorPos();
	CString info;
	info.Format( _T("pos: X=%d, Y=%d"), cursorPos.x, cursorPos.y );

	m_selWnd = m_wndPickerTool.GetSelectedWnd();
	if ( m_selWnd.IsValid() )
	{
		CPoint clientPos = m_wndPickerTool.GetCursorPos(); ::ScreenToClient( m_selWnd.m_hWnd, &clientPos );
		CString clientInfo;
		clientInfo.Format( _T("\nclient: X=%d, Y=%d"), clientPos.x, clientPos.y );
		info += clientInfo;
	}

	SetDlgItemText( IDC_TRACKING_POS_STATIC, info );
}

void CMainDialog::OnTfnTrackFoundWindow( void )
{
	std::tstring info;

	m_selWnd = m_wndPickerTool.GetSelectedWnd();
	if ( m_selWnd.IsValid() )
	{
		CRect windowRect = m_selWnd.GetWindowRect();

		TCHAR title[ 256 ];
		::GetWindowText( m_selWnd.m_hWnd, title, COUNT_OF( title ) - 1 );

		TCHAR className[ 256 ];
		::GetClassName( m_selWnd.m_hWnd, className, COUNT_OF( className ) - 1 );	// found window class name

		// display some information on the found window
		info = str::Format( _T("Window Handle=%08X\r\n%s\r\nTitle=\"%s\"\r\nClass=\"%s\"\r\n\r\nX=%d, Y=%d\tCX=%d, CY=%d\r\n\r\nstyle=0x%08X\r\nstyleEx=0x%08X"),
			m_selWnd.m_hWnd,
			m_selWnd.IsChildWindow() ? _T("CHILD") : _T("POPUP"),
			title,
			className,
			windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(),
			ui::GetStyle( m_selWnd.m_hWnd ),
			ui::GetStyleEx( m_selWnd.m_hWnd ) );
	}

	ui::SetDlgItemText( this, IDC_FOUND_WINDOW_INFO_EDIT, info );
	ui::EnableControl( *this, ID_HIGHLIGHT_WINDOW, m_selWnd.IsValid() );
}

void CMainDialog::OnHighlight( void )
{
	if ( m_selWnd.IsValid() )
	{
		ui::EnableControl( m_hWnd, ID_HIGHLIGHT_WINDOW, false );

		m_pHighlighter.reset( new CWndHighlighter() );
		m_pHighlighter->FlashWnd( m_hWnd, m_selWnd );
	}
	else
		ui::BeepSignal();
}

LRESULT CMainDialog::OnEndHighlight( WPARAM, LPARAM )
{
	// sent when the flash highlight sequence ended
	m_pHighlighter.reset();
	ui::EnableControl( m_hWnd, ID_HIGHLIGHT_WINDOW, true );
	ui::TakeFocus( ::GetDlgItem( m_hWnd, ID_HIGHLIGHT_WINDOW ) );
	return 0L;
}

void CMainDialog::OnOptionChanged( void )
{
	if ( m_frameStyleCombo.m_hWnd != nullptr )		// subclassed?
		UpdateData( TRUE );		// save
}
