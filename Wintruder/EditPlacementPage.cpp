
#include "pch.h"
#include "EditPlacementPage.h"
#include "PromptDialog.h"
#include "AppService.h"
#include "Application.h"
#include "resource.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_page[] = _T("MainDialog\\DetailsSheet\\Placement");
	static const TCHAR entry_autoApply[] = _T("AutoApply");
}


const CSize CEditPlacementPage::m_none( 0, 0 );

CEditPlacementPage::CEditPlacementPage( void )
	: CDetailBasePage( IDD_EDIT_PLACEMENT_PAGE )
	, m_wndRect( 0, 0, 0, 0 )
	, m_oldRect( m_wndRect )
	, m_hWndLastTarget( nullptr )
{
	m_moveTracker.SetToolIconId( ID_MOVE_TOOL );
	m_sizeTracker.SetToolIconId( ID_SIZE_TOOL );
	m_topLeftTracker.SetToolIconId( ID_SIZE_TOP_LEFT_TOOL );
	m_bottomRightTracker.SetToolIconId( ID_SIZE_BOTTOM_RIGHT_TOOL );

	m_moveTracker.LoadTrackCursor( ID_MOVE_TOOL );
	m_sizeTracker.LoadTrackCursor( ID_SIZE_BOTTOM_RIGHT_TOOL );
	m_topLeftTracker.LoadTrackCursor( ID_SIZE_TOP_LEFT_TOOL );
	m_bottomRightTracker.LoadTrackCursor( ID_SIZE_BOTTOM_RIGHT_TOOL );
}

CEditPlacementPage::~CEditPlacementPage()
{
}

bool CEditPlacementPage::IsDirty( void ) const
{
	return !( m_wndRect == m_oldRect );
}

void CEditPlacementPage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	// cancel any previously uncommited movement
	if ( IsDirty() )
		if ( ui::IsValidWindow( m_hWndLastTarget ) )
			::MoveWindow( m_hWndLastTarget, m_oldRect.left, m_oldRect.top, m_oldRect.Width(),
						  m_oldRect.Height(), ::IsWindowVisible( m_hWndLastTarget ) );

	bool valid = targetWnd.IsValid();
	ui::EnableWindow( *this, valid );

	if ( valid )
		m_wndRect = m_oldRect = ui::GetControlRect( targetWnd );
	else
		m_wndRect = m_oldRect = CRect( 0, 0, 0, 0 );

	if ( valid && targetWnd.m_hWnd != m_hWndLastTarget )
		SetSpinRangeLimits();
	m_hWndLastTarget = targetWnd;
	OutputEdits();
	SetModified( false );
}

bool CEditPlacementPage::OnPreTrackMoveCursor( CTrackStatic* pTracking )
{
	if ( !ui::IsValidWindow( m_hWndLastTarget ) )
		return false;

	CRect windowRect;
	::GetWindowRect( m_hWndLastTarget, &windowRect );

	if ( &m_moveTracker == pTracking || &m_topLeftTracker == pTracking )
		::SetCursorPos( windowRect.left, windowRect.top );
	else if ( &m_sizeTracker == pTracking || &m_bottomRightTracker == pTracking )
		::SetCursorPos( windowRect.right, windowRect.bottom );
	else
		return false;

	return true;
}

void CEditPlacementPage::SetSpinRangeLimits( void )
{
	const CWndSpot& targetWnd = app::GetTargetWnd();
	if ( !targetWnd.IsValid() )
		return;

	// set coordinate limits to current monitor enlarged by 10%
	CRect boundsRect = targetWnd.FindMonitorRect();				// monitor corresponding to target window
	boundsRect.InflateRect( boundsRect.Width() * 1 / 10, boundsRect.Height() * 1 / 10 );		// enlarge bounds by 10%

	ui::SetSpinRange( this, IDC_ORG_X_SPIN, boundsRect.left, boundsRect.right );
	ui::SetSpinRange( this, IDC_ORG_Y_SPIN, boundsRect.top, boundsRect.bottom );
	ui::SetSpinRange( this, IDC_WIDTH_SPIN, 0, boundsRect.Width() );
	ui::SetSpinRange( this, IDC_HEIGHT_SPIN, 0, boundsRect.Height() );
	ui::SetSpinRange( this, IDC_LEFT_SPIN, boundsRect.left, boundsRect.right );
	ui::SetSpinRange( this, IDC_TOP_SPIN, boundsRect.top, boundsRect.bottom );
	ui::SetSpinRange( this, IDC_RIGHT_SPIN, boundsRect.left, boundsRect.right );
	ui::SetSpinRange( this, IDC_BOTTOM_SPIN, boundsRect.top, boundsRect.bottom );
}

void CEditPlacementPage::OutputEdits( void )
{
	if ( !app::CheckValidTargetWnd( app::Beep ) )
		return;

	CScopedInternalChange internalChange( this );

	ui::SetDlgItemInt( m_hWnd, IDC_ORG_X_EDIT, m_wndRect.left );
	ui::SetDlgItemInt( m_hWnd, IDC_ORG_Y_EDIT, m_wndRect.top );
	ui::SetDlgItemInt( m_hWnd, IDC_WIDTH_EDIT, abs( m_wndRect.Width() ) );		// normalized width on the fly
	ui::SetDlgItemInt( m_hWnd, IDC_HEIGHT_EDIT, abs( m_wndRect.Height() ) );	// normalized height on the fly
	ui::SetDlgItemInt( m_hWnd, IDC_LEFT_EDIT, m_wndRect.left );
	ui::SetDlgItemInt( m_hWnd, IDC_TOP_EDIT, m_wndRect.top );
	ui::SetDlgItemInt( m_hWnd, IDC_RIGHT_EDIT, m_wndRect.right );
	ui::SetDlgItemInt( m_hWnd, IDC_BOTTOM_EDIT, m_wndRect.bottom );
}

bool CEditPlacementPage::RepositionWnd( void )
{
	CWndSpot* pTargetWnd = app::GetValidTargetWnd( app::Beep );
	if ( nullptr == pTargetWnd )
		return false;

	CRect wndRect = m_wndRect;
	wndRect.NormalizeRect();

	bool changed = !( wndRect == ui::GetControlRect( pTargetWnd->GetSafeHwnd() ) );
	if ( changed )
		pTargetWnd->GetWnd()->MoveWindow( &wndRect, pTargetWnd->IsWindowVisible() );

	if ( IsAutoApply() && IsDirty() )
	{
		m_hWndLastTarget = nullptr;
		OutputTargetWnd();
	}
	return changed;
}

void CEditPlacementPage::HandleInput( void )
{
	OutputEdits();
	RepositionWnd();

	if ( !IsAutoApply() )
	{
		OnFieldModified();
		if ( IsDirty() )
			app::GetSvc().SetDirtyDetails();
	}
}

void CEditPlacementPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		if ( IsDirty() )
			pTargetWnd->GetWnd()->MoveWindow( m_oldRect = m_wndRect, pTargetWnd->IsWindowVisible() );
}

bool CEditPlacementPage::IsAutoApply( void ) const
{
	return IsDlgButtonChecked( IDC_AUTO_APPLY_PLACEMENT_CHECK ) != FALSE;
}

void CEditPlacementPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_moveTracker.m_hWnd;

	DDX_Control( pDX, ID_MOVE_TOOL, m_moveTracker );
	DDX_Control( pDX, ID_SIZE_TOOL, m_sizeTracker );
	DDX_Control( pDX, ID_SIZE_TOP_LEFT_TOOL, m_topLeftTracker );
	DDX_Control( pDX, ID_SIZE_BOTTOM_RIGHT_TOOL, m_bottomRightTracker );

	if ( firstInit )
	{
		EnableToolTips( TRUE );
		CheckDlgButton( IDC_AUTO_APPLY_PLACEMENT_CHECK, AfxGetApp()->GetProfileInt( reg::section_page, reg::entry_autoApply, FALSE ) );
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	CDetailBasePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CEditPlacementPage, CDetailBasePage )
	ON_CONTROL_RANGE( CTrackStatic::TSN_BEGINTRACKING, ID_MOVE_TOOL, ID_SIZE_BOTTOM_RIGHT_TOOL, OnTsnBeginTracking )
	ON_CONTROL_RANGE( CTrackStatic::TSN_ENDTRACKING, ID_MOVE_TOOL, ID_SIZE_BOTTOM_RIGHT_TOOL, OnTsnEndTracking )
	ON_TSN_TRACK( ID_MOVE_TOOL, OnTsnTrack_Move )
	ON_TSN_TRACK( ID_SIZE_TOOL, OnTsnTrack_Size )
	ON_TSN_TRACK( ID_SIZE_TOP_LEFT_TOOL, OnTsnTrack_TopLeft )
	ON_TSN_TRACK( ID_SIZE_BOTTOM_RIGHT_TOOL, OnTsnTrack_BottomRight )
	ON_CONTROL_RANGE( EN_CHANGE, IDC_ORG_X_EDIT, IDC_BOTTOM_EDIT, OnEnChange_Edit )
	ON_BN_CLICKED( IDC_AUTO_APPLY_PLACEMENT_CHECK, OnToggle_AutoApply )
END_MESSAGE_MAP()

void CEditPlacementPage::OnTsnBeginTracking( UINT toolId )
{
	if ( nullptr == app::GetValidTargetWnd() )
	{
		ui::BeepSignal( MB_ICONWARNING );
		static_cast<CTrackStatic*>( GetDlgItem( toolId ) )->CancelTracking();
	}
	else
		m_trackingOldWndRect = m_wndRect;
}

void CEditPlacementPage::OnTsnEndTracking( UINT toolId )
{
	CTrackStatic* pTool = static_cast<CTrackStatic*>( GetDlgItem( toolId ) );
	ASSERT_PTR( pTool );

	bool commit = CTrackStatic::Commit == pTool->GetTrackingResult();
	if ( !commit )
		m_wndRect = m_trackingOldWndRect;		// rollback to the original rect before tracking

	m_wndRect.NormalizeRect();

	OutputEdits();
	RepositionWnd();
}

void CEditPlacementPage::OnTsnTrack_Move( void )
{
	CSize delta = m_moveTracker.GetDelta();
	if ( delta != m_none )
	{
		m_wndRect += delta;
		HandleInput();
	}
}

void CEditPlacementPage::OnTsnTrack_Size( void )
{
	CSize delta = m_sizeTracker.GetDelta();
	if ( delta != m_none )
	{
		CSize newSize = m_wndRect.Size() + delta;
		newSize.cx = std::max<long>( newSize.cx, 0 );
		newSize.cy = std::max<long>( newSize.cy, 0 );
		m_wndRect.BottomRight() = m_wndRect.TopLeft() + newSize;
		HandleInput();
	}
}

void CEditPlacementPage::OnTsnTrack_TopLeft( void )
{
	CSize delta = m_topLeftTracker.GetDelta();
	if ( delta != m_none )
	{
		m_wndRect.TopLeft() += delta;
		HandleInput();
	}
}

void CEditPlacementPage::OnTsnTrack_BottomRight( void )
{
	CSize delta = m_bottomRightTracker.GetDelta();
	if ( delta != m_none )
	{
		m_wndRect.BottomRight() += delta;
		HandleInput();
	}
}

void CEditPlacementPage::OnEnChange_Edit( UINT editId )
{
	if ( m_moveTracker.m_hWnd != nullptr )			// subclassed?
		if ( !IsInternalChange() )
		{
			int newValue = ui::GetDlgItemInt( m_hWnd, editId );
			switch ( editId )
			{
				case IDC_ORG_X_EDIT:	m_wndRect.OffsetRect( newValue - m_wndRect.left, 0 ); break;
				case IDC_ORG_Y_EDIT:	m_wndRect.OffsetRect( 0, newValue - m_wndRect.top ); break;
				case IDC_WIDTH_EDIT:	m_wndRect.right = m_wndRect.left + std::max<long>( newValue, 0 ); break;
				case IDC_HEIGHT_EDIT:	m_wndRect.bottom = m_wndRect.top + std::max<long>( newValue, 0 ); break;
				case IDC_LEFT_EDIT:		m_wndRect.left = newValue; break;
				case IDC_TOP_EDIT:		m_wndRect.top = newValue; break;
				case IDC_RIGHT_EDIT:	m_wndRect.right = newValue; break;
				case IDC_BOTTOM_EDIT:	m_wndRect.bottom = newValue; break;
			}
			HandleInput();
		}
}

void CEditPlacementPage::OnToggle_AutoApply( void )
{
	bool autoApply = IsAutoApply();
	AfxGetApp()->WriteProfileInt( reg::section_page, reg::entry_autoApply, autoApply );
	if ( autoApply )
		RepositionWnd();
}
