
#include "stdafx.h"
#include "MainToolbar.h"
#include "MainFrame.h"
#include "IImageView.h"
#include "Workspace.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/UI/CmdInfoStore.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/StockValuesComboBox.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/StockValuesComboBox.hxx"


const UINT CMainToolbar::s_buttons[] =
{
	ID_FILE_NEW,
	ID_FILE_OPEN,
	ID_FILE_OPEN_ALBUM_FOLDER,
	ID_FILE_SAVE,
		ID_SEPARATOR,
	ID_EDIT_COPY,
		ID_SEPARATOR,
	CM_REFRESH_CONTENT,
	CM_CLEAR_IMAGE_CACHE,
		ID_SEPARATOR,
	CK_FULL_SCREEN,
	CK_SHOW_THUMB_VIEW,
		ID_SEPARATOR,
	IDW_IMAGE_SCALING_COMBO,
	IDW_ZOOM_COMBO,
	ID_ZOOM_NORMAL_100,
	ID_ZOOM_IN,
	ID_ZOOM_OUT,
	IDW_SMOOTHING_MODE_CHECK,
		ID_SEPARATOR,
	ID_TOGGLE_NAVIG_PLAY,
		ID_SEPARATOR,
	ID_NAVIG_SEEK_FIRST,
	ID_NAVIG_SEEK_PREV,
	ID_NAVIG_SEEK_NEXT,
	ID_NAVIG_SEEK_LAST,
		ID_SEPARATOR,
	ID_TOGGLE_NAVIG_DIR_REV,
	ID_TOGGLE_NAVIG_DIR_FWD,
		ID_SEPARATOR,
	ID_TOGGLE_NAVIG_WRAP_MODE,
		ID_SEPARATOR,
	IDW_NAVIG_SLIDER_CTRL
};


CMainToolbar::CMainToolbar( void )
	: CToolbarStrip()
	, m_pScalingCombo( new CEnumComboBox( &ui::GetTags_ImageScalingMode() ) )
	, m_pZoomCombo( new CZoomComboBox() )
{
	ui::MakeStandardControlFont( m_ctrlFont );
}

CMainToolbar::~CMainToolbar()
{
}

bool CMainToolbar::InitToolbar( void )
{
	//LoadToolStrip( IDR_MAINFRAME );		// replaced by GetStrip().AddButtons
	GetStrip().AddButtons( ARRAY_SPAN( s_buttons ) );
	if ( !InitToolbarButtons() )
		return false;

	enum { ScalingModeComboWidth = 130, ZoomComboWidth = 90, SmoothCheckWidth = 65, NavigSliderCtrlWidth = 150 };

	CreateBarCtrl( (CComboBox*)m_pScalingCombo.get(), IDW_IMAGE_SCALING_COMBO, CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL, ScalingModeComboWidth );
	CreateBarCtrl( (CComboBox*)m_pZoomCombo.get(), IDW_ZOOM_COMBO, CBS_DROPDOWN | CBS_DISABLENOSCROLL, ZoomComboWidth );
	CreateBarCtrl( &m_smoothCheck, IDW_SMOOTHING_MODE_CHECK, BS_3STATE, SmoothCheckWidth, PadLeft + 6 );		// push right to avoid overlap on background separator button
	CreateBarCtrl( &m_navigSliderCtrl, IDW_NAVIG_SLIDER_CTRL, TBS_HORZ | TBS_AUTOTICKS | TBS_TRANSPARENTBKGND | TBS_TOOLTIPS, NavigSliderCtrlWidth, 0, 0 );		// no padding

	m_smoothCheck.SetWindowText( _T("Smooth") );

	// setup items for the combos
	OutputScalingMode( CWorkspace::GetData().m_scalingMode );

	OutputZoomPct( 100 );
	return true;
}

template< typename CtrlType >
void CMainToolbar::CreateBarCtrl( CtrlType* pCtrl, UINT ctrlId, DWORD style, int width, int padLeft /*= PadLeft*/, int padRight /*= PadRight*/ )
{
	ASSERT_PTR( pCtrl );
	int buttonPos = CommandToIndex( ctrlId );
	VERIFY( buttonPos != -1 );

	SetButtonInfo( buttonPos, ctrlId, TBBS_SEPARATOR, width );		// set combo width with underlying button as separator

	CRect ctrlRect;
	GetItemRect( buttonPos, &ctrlRect );
	// extra padding on left & right
	ctrlRect.left += padLeft;
	ctrlRect.right -= padRight;

	VERIFY( CreateControl( pCtrl, ctrlId, style, ctrlRect ) );
	pCtrl->SetFont( &m_ctrlFont );
}

template<>
bool CMainToolbar::CreateControl( CComboBox* pComboBox, UINT comboId, DWORD style, const CRect& ctrlRect )
{
	return pComboBox->Create( style | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, comboId ) != FALSE;
}

template<>
bool CMainToolbar::CreateControl( CButton* pButton, UINT buttonId, DWORD style, const CRect& ctrlRect )
{
	return pButton->Create( _T("<ck>"), style | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, buttonId ) != FALSE;
}

template<>
bool CMainToolbar::CreateControl( CSliderCtrl* pSliderCtrl, UINT ctrlId, DWORD style, const CRect& ctrlRect )
{
	return pSliderCtrl->Create( style | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, ctrlId ) != FALSE;
}


bool CMainToolbar::OutputScalingMode( ui::ImageScalingMode scalingMode )
{
	return m_pScalingCombo->SetValue( scalingMode );
}

ui::ImageScalingMode CMainToolbar::InputScalingMode( void ) const
{
	return m_pScalingCombo->GetEnum<ui::ImageScalingMode>();
}

bool CMainToolbar::OutputZoomPct( UINT zoomPct )
{
	return m_pZoomCombo->OutputValue( zoomPct );
}

UINT CMainToolbar::InputZoomPct( ui::ComboField byField ) const
{
	UINT zoomPct;

	if ( !m_pZoomCombo->InputValue( &zoomPct, byField, true ) )
		zoomPct = 0;

	return zoomPct;
}

bool CMainToolbar::OutputNavigRange( UINT imageCount )
{
	enum { ThresholdCount = 30 };

	int maxIndex = std::max( 2u, imageCount - 1 );
	if ( 0 == m_navigSliderCtrl.GetRangeMin() && maxIndex == m_navigSliderCtrl.GetRangeMax() )
		return false;

	m_navigSliderCtrl.SetRange( 0, imageCount - 1, TRUE );
	m_navigSliderCtrl.SetTicFreq( 1 + imageCount / ThresholdCount );
	return true;
}

bool CMainToolbar::OutputNavigPos( int imagePos )
{
	if ( imagePos < 0 || imagePos > m_navigSliderCtrl.GetRangeMax() )
		imagePos = 0;
	if ( m_navigSliderCtrl.GetPos() == imagePos )
		return false;

	m_navigSliderCtrl.SetPos( imagePos );
	return true;
}

int CMainToolbar::InputNavigPos( void ) const
{
	return m_navigSliderCtrl.GetPos();
}

bool CMainToolbar::HandleCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	switch ( cmdId )
	{
		case ID_CM_ESCAPE_KEY:
		case ID_FOCUS_ON_ZOOM_COMBO:
		case ID_FOCUS_ON_SLIDER_CTRL:
			return OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) != FALSE;
	}
	return false;
}


// message handlers

BEGIN_MESSAGE_MAP( CMainToolbar, CToolbarStrip )
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_COMMAND( IDOK, OnOk )
	ON_COMMAND_EX( ID_CM_ESCAPE_KEY, On_EscapeKey )
	ON_COMMAND( ID_FOCUS_ON_ZOOM_COMBO, On_FocusOnZoomCombo )
	ON_COMMAND( ID_FOCUS_ON_SLIDER_CTRL, On_FocusOnSliderCtrl )
	ON_CBN_CLOSEUP( IDW_ZOOM_COMBO, OnCBnCloseUp_ZoomCombo )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnToolTipText_NavigSliderCtrl )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnToolTipText_NavigSliderCtrl )
END_MESSAGE_MAP()

BOOL CMainToolbar::OnEraseBkgnd( CDC* pDC )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	clientRect.bottom += 20;
	::FillRect( pDC->m_hDC, &clientRect, ::GetSysColorBrush( COLOR_3DFACE ) );

	return CToolbarStrip::OnEraseBkgnd( pDC );
}

void CMainToolbar::OnHScroll( UINT sbCode, UINT nPos, CScrollBar* pScrollBar )
{
	CToolbarStrip::OnHScroll( sbCode, nPos, pScrollBar );
	if ( pScrollBar != NULL && pScrollBar->m_hWnd == m_navigSliderCtrl.m_hWnd )
	{
		BOOL doCommit = true;
		int pos;

		switch ( sbCode )
		{
			case SB_LINELEFT:
			case SB_LINERIGHT:
			case SB_PAGELEFT:
			case SB_PAGERIGHT:
			case SB_LEFT:
			case SB_RIGHT:
			case SB_ENDSCROLL:
				pos = m_navigSliderCtrl.GetPos();
				break;
			case SB_THUMBTRACK:
				pos = nPos;
				doCommit = false;
				break;
			default:
				return;
		}

		if ( IImageView* pImageView = app::GetMainFrame()->GetActiveImageView() )
			pImageView->EventNavigSliderPosChanged( SB_THUMBTRACK == sbCode );
	}
}

void CMainToolbar::OnOk( void )
{
	if ( ui::OwnsFocus( m_hWnd ) )
		if ( IImageView* pImageView = app::GetMainFrame()->GetActiveImageView() )
			if ( ui::OwnsFocus( *m_pZoomCombo ) )
				pImageView->RegainFocus( IImageView::Enter, m_pZoomCombo->GetDlgCtrlID() );
			else
				pImageView->RegainFocus( IImageView::Escape );
}

BOOL CMainToolbar::On_EscapeKey( UINT cmdId )
{
	cmdId;
	if ( !ui::OwnsFocus( m_hWnd ) )		// any control on the toolbar has focus?
		return FALSE;					// command not handled

	if ( IImageView* pImageView = app::GetMainFrame()->GetActiveImageView() )
		pImageView->RegainFocus( IImageView::Escape );
	return TRUE;
}

void CMainToolbar::On_FocusOnZoomCombo( void )
{
	if ( !ui::OwnsFocus( *m_pZoomCombo ) && app::GetMainFrame()->MDIGetActive() != NULL )
		m_pZoomCombo->SetFocus();
}

void CMainToolbar::On_FocusOnSliderCtrl( void )
{
	if ( !ui::OwnsFocus( m_navigSliderCtrl ) && m_navigSliderCtrl.IsWindowEnabled() )
		m_navigSliderCtrl.SetFocus();
}

void CMainToolbar::OnCBnCloseUp_ZoomCombo( void )
{
	On_EscapeKey( IDW_ZOOM_COMBO );
}

BOOL CMainToolbar::OnToolTipText_NavigSliderCtrl( UINT, NMHDR* pNmHdr, LRESULT* pResult )
{
	ui::CTooltipTextMessage message( pNmHdr );
	IImageView* pImageView = app::GetMainFrame()->GetActiveImageView();
	if ( !message.IsValidNotification() || message.m_cmdId != IDW_NAVIG_SLIDER_CTRL || NULL == pImageView )
		return FALSE;		// not handled, countinue routing

	if ( !message.AssignTooltipText( pImageView->GetImagePathKey().first.GetPtr() ) )
		return FALSE;		// countinue routing

	*pResult = 0;
	return TRUE;			// message was handled
}
