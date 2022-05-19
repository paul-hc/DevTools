
#include "stdafx.h"
#include "MainDialog.h"
#include "GeneralPage.h"
#include "TreeWndPage.h"
#include "OptionsPage.h"
#include "FindWndDialog.h"
#include "EditCaptionPage.h"
#include "EditIdentPage.h"
#include "EditStylePage.h"
#include "EditStyleExPage.h"
#include "EditPlacementPage.h"
#include "PromptDialog.h"
#include "AppService.h"
#include "Application.h"
#include "resource.h"
#include "wnd/WndFinder.h"
#include "wnd/WndHighlighter.h"
#include "wnd/WndUtils.h"
#include "utl/EnumTags.h"
#include "utl/ScopedValue.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("MainDialog");
	static const TCHAR section_mainSheet[] = _T("MainDialog\\MainSheet");
	static const TCHAR section_detailsSheet[] = _T("MainDialog\\DetailsSheet");
}


enum { TopPct = 50, BottomPct = 50 };

namespace layout
{
	static const CDualLayoutStyle dualLayoutStyles[] =
	{
		{ IDC_MAIN_SHEET, SizeX | pctSizeY( TopPct ), SizeX | SizeY },
		{ IDC_TRACK_TOOL_ICON, pctMoveY( TopPct ), MoveY },
		{ IDC_TRACKING_POS_STATIC, SizeX | pctMoveY( TopPct ), SizeX | MoveY },

		{ CM_HIGHLIGHT_WINDOW, pctMoveY( TopPct ), MoveY },
		{ CM_REFRESH, pctMoveY( TopPct ), MoveY },
		{ CM_FIND_WINDOW, pctMoveY( TopPct ), MoveY },
		{ IDC_EXPAND_DETAILS, MoveX | pctMoveY( TopPct ), MoveX | MoveY },

		{ IDC_BRIEF_INFO_EDIT, SizeX | pctMoveY( TopPct ), SizeX | MoveY },
		{ ID_APPLY_NOW, MoveX | pctMoveY( TopPct ), MoveX | MoveY },
		{ IDC_DETAILS_SHEET, SizeX | pctMoveY( TopPct ) | pctSizeY( BottomPct ), SizeX | MoveY | SizeY },
		{ IDC_WINDOW_DETAILS_GROUP, SizeX | pctMoveY( TopPct ) | pctSizeY( BottomPct ) | CollapsedTop, SizeX | MoveY | SizeY }
	};
}


CMainDialog::CMainDialog( void )
	: CBaseMainDialog( IDD_MAIN_DIALOG )
	, m_autoUpdateTimer( this, TimerAutoUpdate, app::GetOptions()->m_autoUpdateTimeout * 1000 )
	, m_refreshTimer( this, TimerResetRefreshButton, 150 )
{
	GetLayoutEngine().RegisterDualCtrlLayout( ARRAY_PAIR( layout::dualLayoutStyles ) );
	m_regSection = reg::section;
	m_initCollapsed = true;
	m_pSystemTrayInfo.reset( new CSysTrayInfo );
	ui::LoadPopupMenu( m_pSystemTrayInfo->m_popupMenu, IDR_CONTEXT_MENU, app::SysTrayPopup );
	::SetMenuDefaultItem( m_pSystemTrayInfo->m_popupMenu, CM_RESTORE, FALSE );

	app::GetSvc().AddObserver( this );

	m_mainSheet.m_regSection = reg::section_mainSheet;
	m_mainSheet.AddPage( new CGeneralPage );
	m_mainSheet.AddPage( new CTreeWndPage );
	m_mainSheet.AddPage( new COptionsPage );

	m_detailsSheet.m_regSection = reg::section_detailsSheet;
	m_detailsSheet.AddPage( new CEditCaptionPage );
	m_detailsSheet.AddPage( new CEditIdentPage );
	m_detailsSheet.AddPage( new CEditStylePage );
	m_detailsSheet.AddPage( new CEditStyleExPage );
	m_detailsSheet.AddPage( new CEditPlacementPage );

	m_trackWndPicker.LoadTrackCursor( IDR_POINTER_CURSOR );
	m_trackWndPicker.SetTrackIconId( CIconId( ID_TRANSPARENT, LargeIcon ) );

	m_findButton.SetIconId( ID_EDIT_FIND );
	m_findButton.LoadMenu( IDR_CONTEXT_MENU, app::FindSplitButton );
	m_detailsButton.LoadMenu( IDR_CONTEXT_MENU, app::HighlightSplitButton );

	m_optionsToolbar.GetStrip()
		.AddButton( ID_TOP_MOST_CHECK )
		.AddButton( ID_AUTO_HILIGHT_CHECK )
		.AddSeparator()
		.AddButton( ID_IGNORE_HIDDEN_CHECK )
		.AddButton( ID_IGNORE_DISABLED_CHECK )
		.AddSeparator()
		.AddButton( ID_AUTO_UPDATE_CHECK )
		.AddButton( ID_AUTO_UPDATE_REFRESH_CHECK );
}

CMainDialog::~CMainDialog()
{
	app::GetSvc().RemoveObserver( this );
}

const CIcon* CMainDialog::GetDlgIcon( DlgIcon dlgIcon /*= DlgSmallIcon*/ ) const
{
	if ( DlgSmallIcon == dlgIcon )			// custom 4 bit small icon for better contrast
		return ui::GetImageStoresSvc()->RetrieveIcon( CIconId( IDI_APP_SMALL_ICON, SmallIcon ) );

	return __super::GetDlgIcon( dlgIcon );
}

void CMainDialog::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	if ( !IsInternalChange() && !m_trackWndPicker.IsTracking() )
		if ( app::GetOptions()->m_autoHighlight )
			FlashTargetWnd( 1 );

	if ( !targetWnd.HasValidPoint() )
		ui::SetDlgItemText( this, IDC_TRACKING_POS_STATIC, std::tstring() );

	m_briefInfoEdit.SetCurrentWnd( targetWnd );
}

void CMainDialog::OnAppEvent( app::Event appEvent )
{
	switch ( appEvent )
	{
		case app::RefreshWndTree:
			m_refreshButton.SetState( true );		// highlight refresh button
			m_refreshButton.UpdateWindow();
			m_refreshTimer.Start();
			break;
		case app::UpdateTarget:
			SearchUpdateTarget();
			break;
		case app::DirtyChanged:
			ui::EnableWindow( m_applyButton, app::GetSvc().HasDirtyDetails() && !GetLayoutEngine().IsCollapsed() );
			break;
		case app::ToggleAutoUpdate:
			GetAutoUpdateTimer()->SetStarted( app::GetOptions()->m_autoUpdate );
			break;
		case app::WndStateChanged:
			{
				CScopedInternalChange internalChange( this );
				app::GetSvc().SetTargetWnd( app::GetTargetWnd() );			// clear dirty, refresh all pages
			}
			FlashTargetWnd( 2 );
			break;
	}
}

void CMainDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;
	switch ( cmdId )
	{
		case IDC_BRIEF_INFO_EDIT:
			rText = wnd::FormatBriefWndInfo( app::GetTargetWnd().GetSafeHwnd() );
			break;
		case ID_APPLY_NOW:
		{
			if ( m_applyButton.GetRhsPartRect().PtInRect( ui::GetCursorPos( m_applyButton ) ) )
				rText = _T("Apply Options...");
			break;
		}
	}
}

bool CMainDialog::ApplyDetailChanges( bool promptOptions )
{
	if ( !app::CheckValidTargetWnd( app::Report ) )
	{
		app::GetSvc().SetTargetWnd( CWndSpot::m_nullWnd );
		return false;
	}

	bool applied = false;

	CWndSpot& rTargetWnd = app::GetTargetWnd();
	CPromptDialog dialog( rTargetWnd, MakeDirtyString(), this );

	if ( promptOptions )
		if ( dialog.DoModal() != IDOK )
			return false;

	if ( m_detailsSheet.ApplyChanges() )
	{
		dialog.CallSetWindowPos();
		applied = true;
	}

	if ( applied )
		app::GetSvc().PublishEvent( app::WndStateChanged );
	else
		app::GetSvc().SetTargetWnd( rTargetWnd );			// clear dirty, refresh all pages
	return applied;
}

void CMainDialog::DrillDownDetail( DetailPage detailPage )
{
	GetLayoutEngine().SetCollapsed( false );
	m_detailsSheet.SetActivePage( detailPage );
}

void CMainDialog::FlashTargetWnd( int flashCount )
{
	if ( m_pFlashHighlighter.get() != NULL || m_trackWndPicker.IsTracking() || IsInternalChange() )
		return;

	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd( app::Beep ) )
	{
		ui::EnableWindow( m_highlightButton, false );
		m_highlightButton.UpdateWindow();
		m_pFlashHighlighter.reset( new CWndHighlighter );
		m_pFlashHighlighter->FlashWnd( m_hWnd, *pTargetWnd, flashCount );
	}
}

void CMainDialog::AutoUpdateRefresh( void )
{
	CmUpdateTick();
}

std::tstring CMainDialog::MakeDirtyString( void ) const
{
	static const CEnumTags pageTags( _T("Caption|Identifier|Style|Extended Style|Placement") );
	std::tstring laundryList; laundryList.reserve( 128 );

	for ( int i = 0, count = m_detailsSheet.GetPageCount(); i != count; ++i )
		if ( IWndDetailObserver* pDetail = m_detailsSheet.GetPageAs< IWndDetailObserver >( i ) )
			if ( pDetail->CanNotify() && pDetail->IsDirty() )
				stream::Tag( laundryList, pageTags.GetUiTags()[ i ], _T(", ") );

	return laundryList;
}

void CMainDialog::LoadFromRegistry( void )
{
	__super::LoadFromRegistry();
	m_searchPattern.Load();
}

void CMainDialog::SaveToRegistry( void )
{
	m_searchPattern.Save();
	__super::SaveToRegistry();
}

void CMainDialog::OnCollapseChanged( bool collapsed )
{
	__super::OnCollapseChanged( collapsed );

	m_detailsButton.SetIconId( collapsed ? ID_EXPAND : ID_COLLAPSE );
	ui::EnableWindow( m_applyButton, app::GetSvc().HasDirtyDetails() && !GetLayoutEngine().IsCollapsed() );
}

void CMainDialog::SearchUpdateTarget( void )
{
	CWndFinder finder;
	CWndSpot foundWnd = finder.FindUpdateTarget();
	if ( foundWnd.IsNull() || foundWnd.IsValid() )
		app::GetSvc().SetTargetWnd( foundWnd );

	if ( foundWnd.IsNull() )
		Beep( 2000, 50 );
}

void CMainDialog::SearchWindow( void )
{
	m_mainSheet.SetActivePage( WindowsTopPage );

	CWndFinder finder;
	if ( HWND hFoundWnd = finder.FindWindow( m_searchPattern, m_searchPattern.m_fromBeginning ? NULL : app::GetValidTargetWnd()->GetSafeHwnd() ) )
		app::GetSvc().SetTargetWnd( hFoundWnd );
	else
		AfxMessageBox( str::Format( _T("%s\n\n%s"), str::Load( IDS_WND_NOT_FOUND ).c_str(), m_searchPattern.FormatNotFound().c_str() ).c_str() );
}

void CMainDialog::SearchNextWindow( bool forward /*= true*/ )
{
	CScopedValue< bool > scopedForward( &m_searchPattern.m_forward, forward );
	CScopedValue< bool > scopedFromSel( &m_searchPattern.m_fromBeginning, false );

	SearchWindow();
}

void CMainDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_trackWndPicker.m_hWnd;

	CScopedInternalChange internalChange( this );

	DDX_Control( pDX, IDC_TRACK_TOOL_ICON, m_trackWndPicker );
	DDX_Control( pDX, IDC_BRIEF_INFO_EDIT, m_briefInfoEdit );
	DDX_Control( pDX, CM_HIGHLIGHT_WINDOW, m_highlightButton );
	DDX_Control( pDX, CM_REFRESH, m_refreshButton );
	DDX_Control( pDX, CM_FIND_WINDOW, m_findButton );
	DDX_Control( pDX, IDC_EXPAND_DETAILS, m_detailsButton );
	DDX_Control( pDX, ID_APPLY_NOW, m_applyButton );
	m_mainSheet.DDX_DetailSheet( pDX, IDC_MAIN_SHEET );
	m_detailsSheet.DDX_DetailSheet( pDX, IDC_DETAILS_SHEET );
	m_optionsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignTop );

	if ( firstInit )
	{
		m_detailsButton.SetIconId( GetLayoutEngine().IsCollapsed() ? ID_EXPAND : ID_COLLAPSE );
		m_detailsSheet.SetSingleLineTab();
		app::GetMainTooltip()->AddTool( this, IDD_MAIN_DIALOG, wnd::GetCaptionRect( *this ), IDD_MAIN_DIALOG );
		m_detailsSheet.GetTabControl()->GetToolTips()->ModifyStyle( 0, TTS_ALWAYSTIP );
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_COMMAND( IDC_EXPAND_DETAILS, OnToggleCollapseDetails )
	ON_TSN_BEGINTRACKING( IDC_TRACK_TOOL_ICON, OnTsnBeginTracking_WndPicker )
	ON_TSN_ENDTRACKING( IDC_TRACK_TOOL_ICON, OnTsnEndTracking_WndPicker )
	ON_TSN_TRACK( IDC_TRACK_TOOL_ICON, OnTsnTrack_WndPicker )
	ON_COMMAND( CM_HIGHLIGHT_WINDOW, OnHighlight )
	ON_REGISTERED_MESSAGE( CTimerSequenceHook::WM_ENDTIMERSEQ, OnEndTimerSequence )
	ON_COMMAND( CM_FIND_WINDOW, OnFindWindow )
	ON_COMMAND( CM_FIND_NEXT_WINDOW, OnFindNextWindow )
	ON_COMMAND( CM_FIND_PREV_WINDOW, OnFindPrevWindow )
	ON_COMMAND( CM_UPDATE_TICK, CmUpdateTick )
	ON_COMMAND_RANGE( CM_VIEW_GENERAL, CM_VIEW_OPTIONS, CmViewMainPage )
	ON_COMMAND_RANGE( CM_EDIT_CAPTION, CM_EDIT_PLACE, CmEditDetails )
	ON_BN_CLICKED( ID_APPLY_NOW, OnApplyNow )
	ON_SBN_RIGHTCLICKED( ID_APPLY_NOW, OnSbnRightClicked_ApplyNow )
END_MESSAGE_MAP()

BOOL CMainDialog::OnInitDialog( void )
{
	__super::OnInitDialog();

	ui::SetTopMost( m_hWnd, app::GetOptions()->m_keepTopmost );
	if ( app::GetOptions()->m_autoUpdate )
		m_autoUpdateTimer.Start();

	return TRUE;
}

void CMainDialog::OnDestroy( void )
{
	app::GetMainTooltip()->DestroyWindow();
	__super::OnDestroy();
}

void CMainDialog::OnTimer( UINT_PTR eventId )
{
	if ( m_autoUpdateTimer.IsHit( eventId ) )
		AutoUpdateRefresh();
	else if ( m_refreshTimer.IsHit( eventId ) )
	{
		m_refreshTimer.Stop();
		m_refreshButton.SetState( false );
	}
	else
		__super::OnTimer( eventId );
}

HBRUSH CMainDialog::OnCtlColor( CDC* dc, CWnd* pWnd, UINT ctlColor )
{
	HBRUSH hBrush = __super::OnCtlColor( dc, pWnd, ctlColor );

	if ( ctlColor == CTLCOLOR_STATIC && pWnd == &m_briefInfoEdit )
		dc->SetTextColor( app::GetTargetWnd().IsValid() ? HotFieldColor : StaleWndColor );

	return hBrush;
}

void CMainDialog::OnToggleCollapseDetails( void )
{
	GetLayoutEngine().ToggleCollapsed();
}

void CMainDialog::OnTsnBeginTracking_WndPicker( void )
{
	if ( app::GetOptions()->m_hideOnTrack )
		app::GetApplication()->ShowAppPopups( false );
}

void CMainDialog::OnTsnEndTracking_WndPicker( void )
{
	if ( app::GetOptions()->m_hideOnTrack )
		app::GetApplication()->ShowAppPopups( true );
}

void CMainDialog::OnTsnTrack_WndPicker( void )
{
	const CWndSpot& pickedWnd = m_trackWndPicker.GetSelectedWnd();
	const CWndSpot& targetWnd = app::GetTargetWnd();

	if ( !pickedWnd.Equals( targetWnd ) )
		app::GetSvc().SetTargetWnd( pickedWnd );

	CPoint cursorPos = m_trackWndPicker.GetCursorPos();
	std::tstring info = str::Format( _T("pos: X=%d, Y=%d"), cursorPos.x, cursorPos.y );

	if ( pickedWnd.IsValid() )
	{
		CPoint clientPos = cursorPos;
		::ScreenToClient( pickedWnd.m_hWnd, &clientPos );

		info += str::Format( _T("  client: X=%d, Y=%d"), clientPos.x, clientPos.y );
	}

	ui::SetDlgItemText( this, IDC_TRACKING_POS_STATIC, info );
}

void CMainDialog::OnHighlight( void )
{
	FlashTargetWnd( 4 );
}

LRESULT CMainDialog::OnEndTimerSequence( WPARAM eventId, LPARAM )
{
	switch ( eventId )
	{
		case CWndHighlighter::FlashTimerEvent:		// sent when the flash highlight sequence ended
			m_pFlashHighlighter.reset();
			ui::EnableWindow( m_highlightButton, true );
			m_highlightButton.SetState( false );	// un-highlight
			break;
	}
	return 0L;
}

void CMainDialog::CmUpdateTick( void )
{
	if ( app::GetOptions()->m_autoUpdateRefresh )
		app::GetSvc().PublishEvent( app::RefreshWndTree );

	app::GetSvc().PublishEvent( app::UpdateTarget );
}

void CMainDialog::OnFindWindow( void )
{
	CFindWndDialog dialog( &m_searchPattern, this );
	if ( IDOK == dialog.DoModal() )
	{
		if ( m_searchPattern.m_refreshNow )
			app::GetSvc().PublishEvent( app::RefreshWndTree );

		SearchWindow();
	}
}

void CMainDialog::OnFindNextWindow( void )
{
	SearchNextWindow( true );
}

void CMainDialog::OnFindPrevWindow( void )
{
	SearchNextWindow( false );
}

void CMainDialog::CmViewMainPage( UINT cmdId )
{
	switch ( cmdId )
	{
		case CM_VIEW_GENERAL:	m_mainSheet.SetActivePage( GeneralTopPage );break;
		case CM_VIEW_WND_TREE:	m_mainSheet.SetActivePage( WindowsTopPage );break;
		case CM_VIEW_OPTIONS:	m_mainSheet.SetActivePage( OptionsTopPage );break;
	}
}

void CMainDialog::CmEditDetails( UINT cmdId )
{
	switch ( cmdId )
	{
		case CM_EDIT_CAPTION:	DrillDownDetail( CaptionDetail ); break;
		case CM_EDIT_IDENT:		DrillDownDetail( IdMenuDetail ); break;
		case CM_EDIT_STYLE:		DrillDownDetail( StyleDetail ); break;
		case CM_EDIT_STYLE_EX:	DrillDownDetail( ExtendedStyleDetail ); break;
		case CM_EDIT_PLACE:		DrillDownDetail( PlacementDetail ); break;
	}
}

void CMainDialog::OnApplyNow( void )
{
	ApplyDetailChanges( false );
}

void CMainDialog::OnSbnRightClicked_ApplyNow( void )
{
	ApplyDetailChanges( true );
}
