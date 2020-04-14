
#include "stdafx.h"
#include "AlbumDialogBar.h"
#include "AlbumDoc.h"
#include "AlbumImageView.h"
#include "Application.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/UI/CmdInfoStore.h"
#include "utl/UI/StockValuesComboBox.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/StockValuesComboBox.hxx"


CAlbumDialogBar::CAlbumDialogBar( void )
	: CDialogBar()
	, m_pSlideDelayCombo( new CDurationComboBox() )
	, m_pAlbumView( NULL )
{
}

CAlbumDialogBar::~CAlbumDialogBar()
{
}

void CAlbumDialogBar::InitAlbumImageView( CAlbumImageView* pAlbumView )
{
	m_pAlbumView = pAlbumView;
	ui::EnableWindow( m_hWnd );
}

void CAlbumDialogBar::ShowBar( bool show )
{
	if ( show != HasFlag( GetStyle(), WS_VISIBLE ) )
		GetParentFrame()->ShowControlBar( this, show, FALSE );
}

bool CAlbumDialogBar::SetCurrentPos( int currIndex, bool forceLoad /*= false*/ )
{
	if ( !m_pAlbumView->IsValidIndex( currIndex ) )
		return false;

	if ( currIndex != m_pAlbumView->GetSlideData().GetCurrentIndex() || forceLoad )
	{
		m_pAlbumView->RefSlideData().SetCurrentIndex( currIndex, true );
		m_pAlbumView->UpdateImage();		// this will also feedback on OnCurrPosChanged()
	}
	else if ( m_scrollSpin.GetPos() - 1 != currIndex )
		m_scrollSpin.SetPos( currIndex + 1 );

	return true;
}

bool CAlbumDialogBar::InputSlideDelay( ui::ComboField byField )
{
	UINT slideDelay;
	if ( !m_pSlideDelayCombo->InputValue( &slideDelay, byField, true ) )
		return false;		// invalid delay

	m_pAlbumView->SetSlideDelay( slideDelay );
	m_pAlbumView->OnSlideDataChanged( false );		// don't set modified flag for this change
	OnSlideDelayChanged();			// update the content of combo box
	return true;
}

// displayed index is actually 1-based
void CAlbumDialogBar::OnNavRangeChanged( void )
{
	int imageCount = (int)m_pAlbumView->GetDocument()->GetImageCount();
	m_scrollSpin.SetRange32( 1, imageCount );
	ui::SetDlgItemText( m_hWnd, IDC_NAV_COUNT_STATIC, str::Format( _T("%d image(s)"), imageCount ) );
}

void CAlbumDialogBar::OnCurrPosChanged( void )
{
	int currIndex = m_pAlbumView->GetSlideData().GetCurrentIndex();
	bool valid = m_pAlbumView->IsValidIndex( currIndex );
	std::tstring imageFileInfo;

	if ( valid )
		if ( const CFileAttr* pFileAttr = &m_pAlbumView->GetDocument()->m_fileList.GetFileAttr( currIndex ) )
			imageFileInfo = str::Format( _T("%s (%s)"), pFileAttr->GetPath().FormatPretty().c_str(), pFileAttr->FormatFileSize( 1, _T("%s B") ).c_str() );

	ui::EnableWindow( m_navEdit, valid );

	if ( valid )
	{
		// slider displayed index is actually 1-based
		if ( m_scrollSpin.GetPos() - 1 != currIndex )
			m_scrollSpin.SetPos( currIndex + 1 );
	}
	else
		ui::SetWindowText( m_navEdit, std::tstring() );

	ui::SetWindowText( m_fileNameEdit, imageFileInfo );
}

void CAlbumDialogBar::OnSlideDelayChanged( void )
{
	m_pSlideDelayCombo->OutputValue( m_pAlbumView->GetSlideData().m_slideDelay );
}

void CAlbumDialogBar::LayoutControls( void )
{
	CRect rectBar, rectEdit;
	if ( NULL == m_fileNameEdit.m_hWnd )
		return;

	// layout the image file name edit to right bound of the dialog-bar
	GetWindowRect( rectBar );
	m_fileNameEdit.GetWindowRect( rectEdit );
	rectEdit.right = rectBar.right - 2;
	ScreenToClient( rectEdit );
	m_fileNameEdit.MoveWindow( rectEdit );
}

BOOL CAlbumDialogBar::PreTranslateMessage( MSG* pMsg )
{
// it doesn't hit here
//	if ( CAccelTable::IsKeyMessage( pMsg ) )
//		if ( app::GetApp()->m_sharedAccel.TranslateIfOwnsFocus( pMsg, m_hWnd, m_hWnd ) )
//			return TRUE;

	return CDialogBar::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumDialogBar, CDialogBar )
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_VSCROLL()
	ON_COMMAND( IDOK, OnOk )
	ON_COMMAND( ID_CM_ESCAPE_KEY, On_EscapeKey )
	ON_CBN_SELCHANGE( IDC_PLAY_DELAY_COMBO, OnCBnSelChange_SlideDelay )
	ON_CBN_CLOSEUP( IDC_PLAY_DELAY_COMBO, OnCBnCloseUp_SlideDelay )
	ON_CN_INPUTERROR( IDC_PLAY_DELAY_COMBO, OnCBnInputError_SlideDelay )
	ON_UPDATE_COMMAND_UI( IDC_PLAY_DELAY_COMBO, OnUpdate_SlideDelay )
	ON_EN_KILLFOCUS( IDC_SEEK_CURR_POS_EDIT, OnEnKillFocus_SeekCurrPos )
	ON_MESSAGE( WM_INITDIALOG, HandleInitDialog )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText )
END_MESSAGE_MAP()

LRESULT CAlbumDialogBar::HandleInitDialog( WPARAM wParam, LPARAM lParam )
{
	CDialogBar::HandleInitDialog( wParam, lParam );

	VERIFY( m_pSlideDelayCombo->SubclassDlgItem( IDC_PLAY_DELAY_COMBO, this ) );
	VERIFY( m_navEdit.SubclassDlgItem( IDC_SEEK_CURR_POS_EDIT, this ) );
	VERIFY( m_scrollSpin.SubclassDlgItem( IDC_SCROLL_POS_SPIN, this ) );
	VERIFY( m_infoStatic.SubclassDlgItem( IDC_NAV_COUNT_STATIC, this ) );
	VERIFY( m_fileNameEdit.SubclassDlgItem( IDC_CURR_FILE_EDIT, this ) );

	LOGFONT logFont;
	if ( GetFont()->GetLogFont( &logFont ) )
	{
		logFont.lfWeight = FW_BOLD;
		if ( m_boldFont.CreateFontIndirect( &logFont ) )
		{
			m_infoStatic.SetFont( &m_boldFont );
			m_fileNameEdit.SetFont( &m_boldFont );
		}
	}
	LayoutControls();
	return TRUE;
}

void CAlbumDialogBar::OnSize( UINT sizeType, int cx, int cy )
{
	CDialogBar::OnSize( sizeType, cx, cy );
	LayoutControls();
}

HBRUSH CAlbumDialogBar::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor )
{
	HBRUSH hBrush = CDialogBar::OnCtlColor( pDC, pWnd, ctlColor );

	if ( CTLCOLOR_STATIC == ctlColor )
		if ( pWnd->m_hWnd == m_infoStatic.m_hWnd )
			pDC->SetTextColor( RGB( 0, 0, 255 ) );
		else if ( pWnd->m_hWnd == m_fileNameEdit.m_hWnd )
			pDC->SetTextColor( RGB( 128, 0, 0 ) );

	return hBrush;
}

void CAlbumDialogBar::OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	CDialogBar::OnVScroll( sbCode, pos, pScrollBar );

	if ( pScrollBar->m_hWnd == m_scrollSpin.m_hWnd )
		switch ( sbCode )
		{
			case SB_THUMBPOSITION:
				m_pAlbumView->RefSlideData().SetCurrentIndex( pos - 1 );
				OnCurrPosChanged();
				break;
			case SB_ENDSCROLL:
				m_pAlbumView->UpdateImage();		// this will also feedback on OnCurrPosChanged()
				break;
		}
}

void CAlbumDialogBar::OnOk( void )
{
	if ( !ui::OwnsFocus( m_hWnd ) )
		return;

	if ( ui::OwnsFocus( *m_pSlideDelayCombo ) )
	{
		if ( InputSlideDelay( ui::ByEdit ) )
			m_pAlbumView->SetFocus();
		else
			ui::BeepSignal();
	}
	else if ( ui::OwnsFocus( m_navEdit ) )
	{
		if ( !SetCurrentPos( m_scrollSpin.GetPos() - 1, true ) )
			ui::BeepSignal();
	}
}

void CAlbumDialogBar::On_EscapeKey( void )
{
	if ( ui::OwnsFocus( m_hWnd ) )
		m_pAlbumView->SetFocus();
}

void CAlbumDialogBar::OnCBnSelChange_SlideDelay( void )
{
	InputSlideDelay( ui::BySel );
}

void CAlbumDialogBar::OnCBnCloseUp_SlideDelay( void )
{
	m_pAlbumView->SetFocus();
}

void CAlbumDialogBar::OnCBnInputError_SlideDelay( void )
{
	OnSlideDelayChanged();
}

void CAlbumDialogBar::OnEnKillFocus_SeekCurrPos( void )
{
	BOOL validInput;
	int currIndex = GetDlgItemInt( IDC_SEEK_CURR_POS_EDIT, &validInput ) - 1;

	if ( !validInput || currIndex != m_pAlbumView->GetSlideData().GetCurrentIndex() )
		OnCurrPosChanged();
}

void CAlbumDialogBar::OnUpdate_SlideDelay( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( true );
}

BOOL CAlbumDialogBar::OnToolTipText( UINT, NMHDR* pNmHdr, LRESULT* pResult )
{
	ui::CTooltipTextMessage message( pNmHdr );
	if ( !message.IsValidNotification() || message.m_cmdId != IDC_CURR_FILE_EDIT )
		return FALSE;		// not handled

	if ( !message.AssignTooltipText( m_pAlbumView->GetImagePathKey().first.Get() ) )
		return FALSE;
	*pResult = 0;
	return TRUE;	// message was handled
}
