
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
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	struct FormatZoomPct
	{
		std::tstring operator()( UINT zoomPct ) const { return str::Format( _T("%d %%"), zoomPct ); }
	};
}


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
	IDW_AUTO_IMAGE_SIZE_COMBO,
	IDW_ZOOM_COMBO,
	IDW_SMOOTHING_MODE_CHECK,
	CM_ZOOM_NORMAL,
	CM_ZOOM_IN,
	CM_ZOOM_OUT,
		ID_SEPARATOR,
	CM_NAV_PLAY,
		ID_SEPARATOR,
	CM_NAV_BEGIN,
	CM_NAV_PREV,
	CM_NAV_NEXT,
	CM_NAV_END,
		ID_SEPARATOR,
	CM_NAV_DIR_REV,
	CM_NAV_DIR_FWD,
		ID_SEPARATOR,
	CM_NAV_CIRCULAR,
		ID_SEPARATOR,
	IDW_NAV_SLIDER
};


CMainToolbar::CMainToolbar( void )
	: CToolbarStrip()
{
	ui::MakeStandardControlFont( m_ctrlFont );
}

CMainToolbar::~CMainToolbar()
{
}

bool CMainToolbar::InitToolbar( void )
{
	//LoadToolStrip( IDR_MAINFRAME );		// replaced by GetStrip().AddButtons
	GetStrip().AddButtons( s_buttons, COUNT_OF( s_buttons ) );
	if ( !InitToolbarButtons() )
		return false;

	enum { AutoImageSizeComboWidth = 130, ZoomComboWidth = 90, SmoothCheckWidth = 67 };

	VERIFY( CreateBarCtrl( &m_autoImageSizeCombo, IDW_AUTO_IMAGE_SIZE_COMBO, CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL, AutoImageSizeComboWidth ) );
	VERIFY( CreateBarCtrl( &m_zoomCombo, IDW_ZOOM_COMBO, CBS_DROPDOWN | CBS_DISABLENOSCROLL, ZoomComboWidth ) );
	VERIFY( CreateBarCtrl( &m_smoothCheck, IDW_SMOOTHING_MODE_CHECK, BS_CHECKBOX | WS_DLGFRAME, SmoothCheckWidth ) );
	m_smoothCheck.SetWindowText( _T("Smooth") );

	// setup items for the combos
	ui::WriteComboItems( m_autoImageSizeCombo, ui::GetTags_AutoImageSize().GetUiTags() );
	OutputAutoSize( CWorkspace::GetData().m_autoImageSize );

	ui::WriteComboItems( m_zoomCombo, ui::CStdZoom::Instance().m_zoomPcts, func::FormatZoomPct() );
	OutputZoomPct( 100 );

	// create the slider control
	int buttonPos = CommandToIndex( IDW_NAV_SLIDER );
	SetButtonInfo( buttonPos, IDW_NAV_SLIDER, TBBS_SEPARATOR, 150 );					// set slider width with underlying button as separator
	CRect ctrlRect;
	GetItemRect( buttonPos, &ctrlRect );
	VERIFY( m_navigSlider.Create( TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, IDW_NAV_SLIDER ) );
	m_navigSlider.SetFont( &m_ctrlFont );
	return true;
}

template< typename CtrlType >
bool CMainToolbar::CreateBarCtrl( CtrlType* pCtrl, UINT ctrlId, DWORD style, int width, UINT tbButtonStyle /*= TBBS_SEPARATOR*/ )
{
	ASSERT_PTR( pCtrl );
	int buttonPos = CommandToIndex( ctrlId );
	if ( -1 == buttonPos )
		return false;

	SetButtonInfo( buttonPos, ctrlId, tbButtonStyle, width );		// set combo width with underlying button as separator

	CRect ctrlRect;
	GetItemRect( buttonPos, &ctrlRect );
	ctrlRect.left += 2;							// some more gap on left and right
	ctrlRect.right -= 5;

	VERIFY( CreateControl( pCtrl, ctrlId, style, ctrlRect ) );
	pCtrl->SetFont( &m_ctrlFont );
	return true;
}

template<>
bool CMainToolbar::CreateControl( CComboBox* pCombo, UINT comboId, DWORD style, const CRect& ctrlRect )
{
	return pCombo->Create( style | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, comboId ) != FALSE;
}

template<>
bool CMainToolbar::CreateControl( CButton* pButton, UINT buttonId, DWORD style, const CRect& ctrlRect )
{
	return pButton->Create( _T("<ck>"), style | WS_VISIBLE | WS_TABSTOP, ctrlRect, this, buttonId ) != FALSE;
}

bool CMainToolbar::OutputAutoSize( ui::AutoImageSize autoImageSize )
{
	if ( m_autoImageSizeCombo.GetCurSel() == autoImageSize )
		return false;

	m_autoImageSizeCombo.SetCurSel( autoImageSize );
	return true;
}

ui::AutoImageSize CMainToolbar::InputAutoSize( void ) const
{
	return static_cast< ui::AutoImageSize >( m_autoImageSizeCombo.GetCurSel() );
}

bool CMainToolbar::OutputZoomPct( UINT zoomPct )
{
	return ui::SetComboEditText( m_zoomCombo, func::FormatZoomPct()( zoomPct ) ).first;
}

UINT CMainToolbar::InputZoomPct( ui::ComboField byField ) const
{
	UINT zoomPct;
	return num::ParseNumber( zoomPct, ui::GetComboSelText( m_zoomCombo, byField ) ) ? zoomPct : 0;
}

bool CMainToolbar::OutputNavigRange( UINT imageCount )
{
	enum { ThresholdCount = 30 };

	int maxIndex = std::max( 2u, imageCount - 1 );
	if ( 0 == m_navigSlider.GetRangeMin() && maxIndex == m_navigSlider.GetRangeMax() )
		return false;

	m_navigSlider.SetRange( 0, imageCount - 1, TRUE );
	m_navigSlider.SetTicFreq( 1 + imageCount / ThresholdCount );
	return true;
}

bool CMainToolbar::OutputNavigPos( int imagePos )
{
	if ( imagePos < 0 || imagePos > m_navigSlider.GetRangeMax() )
		imagePos = 0;
	if ( m_navigSlider.GetPos() == imagePos )
		return false;

	m_navigSlider.SetPos( imagePos );
	return true;
}

int CMainToolbar::InputNavigPos( void ) const
{
	return m_navigSlider.GetPos();
}

bool CMainToolbar::HandleCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	switch ( cmdId )
	{
		case CM_ESCAPE_KEY:
		case CM_FOCUS_ZOOM:
		case CM_FOCUS_SLIDER:
			return OnCmdMsg( cmdId, code, pExtra, pHandlerInfo ) != FALSE;
	}
	return false;
}


// message handlers

BEGIN_MESSAGE_MAP( CMainToolbar, CToolbarStrip )
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_COMMAND( IDOK, OnOk )
	ON_COMMAND_EX( CM_ESCAPE_KEY, CmEscapeKey )
	ON_COMMAND( CM_FOCUS_ZOOM, CmFocusZoom )
	ON_COMMAND( CM_FOCUS_SLIDER, CmFocusSlider )
	ON_CBN_CLOSEUP( IDW_ZOOM_COMBO, OnCBnCloseUp_ZoomCombo )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText_NavigSlider )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText_NavigSlider )
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
	if ( pScrollBar != NULL && pScrollBar->m_hWnd == m_navigSlider.m_hWnd )
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
				pos = m_navigSlider.GetPos();
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
			if ( ui::OwnsFocus( m_zoomCombo ) )
				pImageView->RegainFocus( IImageView::Enter, m_zoomCombo.GetDlgCtrlID() );
			else
				pImageView->RegainFocus( IImageView::Escape );
}

BOOL CMainToolbar::CmEscapeKey( UINT cmdId )
{
	cmdId;
	if ( !ui::OwnsFocus( m_hWnd ) )		// any control on the toolbar has focus?
		return FALSE;					// command not handled

	if ( IImageView* pImageView = app::GetMainFrame()->GetActiveImageView() )
		pImageView->RegainFocus( IImageView::Escape );
	return TRUE;
}

void CMainToolbar::CmFocusZoom( void )
{
	if ( !ui::OwnsFocus( m_zoomCombo ) && app::GetMainFrame()->MDIGetActive() != NULL )
		m_zoomCombo.SetFocus();
}

void CMainToolbar::CmFocusSlider( void )
{
	if ( !ui::OwnsFocus( m_navigSlider ) && m_navigSlider.IsWindowEnabled() )
		m_navigSlider.SetFocus();
}

void CMainToolbar::OnCBnCloseUp_ZoomCombo( void )
{
	CmEscapeKey( IDW_ZOOM_COMBO );
}

BOOL CMainToolbar::OnToolTipText_NavigSlider( UINT, NMHDR* pNmHdr, LRESULT* pResult )
{
	ui::CTooltipTextMessage message( pNmHdr );
	IImageView* pImageView = app::GetMainFrame()->GetActiveImageView();
	if ( !message.IsValidNotification() || message.m_cmdId != IDW_NAV_SLIDER || NULL == pImageView )
		return FALSE;		// not handled

	if ( !message.AssignTooltipText( pImageView->GetImagePathKey().first.GetPtr() ) )
		return FALSE;

	*pResult = 0;
	return TRUE;			// message was handled
}
