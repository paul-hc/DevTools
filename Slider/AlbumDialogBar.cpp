
#include "pch.h"
#include "AlbumDialogBar.h"
#include "AlbumDoc.h"
#include "AlbumImageView.h"
#include "FileAttr.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/StockValuesComboBox.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/StockValuesComboBox.hxx"


namespace layout
{
	enum { CommentsPct = 60, ListPct = 100 - CommentsPct };

	static const CLayoutStyle s_styles[] =
	{
		{ IDW_CURR_IMAGE_PATH_LABEL, SizeX }
	};
}


CAlbumDialogPane::CAlbumDialogPane( void )
	: CLayoutPaneDialog()
	, m_pToolbar( new CDialogToolBar() )
	, m_pSlideDelayCombo( new CDurationComboBox() )
	, m_pImagePathEdit( new CTextEdit( false ) )
	, m_pAlbumView( nullptr )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );

	//m_fillToolBarBkgnd = true;
	m_showControlBarMenu = false;

	m_pToolbar->GetStrip()
		.AddButton( ID_EDIT_ALBUM );
}

CAlbumDialogPane::~CAlbumDialogPane()
{
}

void CAlbumDialogPane::InitAlbumImageView( CAlbumImageView* pAlbumView ) implement
{
	m_pAlbumView = pAlbumView;
	ui::EnableWindow( m_hWnd );
}

void CAlbumDialogPane::ShowBar( bool show ) implement
{
	ShowPane( show, false, false );
}

// displayed index is actually 1-based
void CAlbumDialogPane::OnNavRangeChanged( void ) implement
{
	if ( nullptr == m_pAlbumView )
		return;		// not yet initialized

	int imageCount = (int)m_pAlbumView->GetDocument()->GetImageCount();
	m_scrollSpin.SetRange32( 1, imageCount );
	ui::SetDlgItemText( m_hWnd, IDW_NAV_COUNT_LABEL, str::Format( _T("/ %s"), num::FormatNumber( imageCount, str::GetUserLocale() ).c_str() ) );
}

void CAlbumDialogPane::OnCurrPosChanged( void ) implement
{
	if ( nullptr == m_pAlbumView )
		return;		// not yet initialized

	int currIndex = m_pAlbumView->GetSlideData().GetCurrentIndex();
	bool valid = m_pAlbumView->IsValidIndex( currIndex );
	std::tstring imageFileInfo;

	if ( valid )
		if ( const CFileAttr* pFileAttr = m_pAlbumView->GetDocument()->GetModel()->GetFileAttr( currIndex ) )
			imageFileInfo = pFileAttr->GetPath().FormatPretty();

	ui::EnableWindow( m_navPosEdit, valid );

	if ( valid )
	{
		// slider displayed index is actually 1-based
		if ( m_scrollSpin.GetPos() - 1 != currIndex )
			m_scrollSpin.SetPos( currIndex + 1 );
	}
	else
		ui::SetWindowText( m_navPosEdit, std::tstring() );

	m_pImagePathEdit->SetText( imageFileInfo );
}

void CAlbumDialogPane::OnSlideDelayChanged( void ) implement
{
	if ( nullptr == m_pAlbumView )
		return;		// not yet initialized

	m_pSlideDelayCombo->OutputValue( m_pAlbumView->GetSlideData().m_slideDelay );
}

void CAlbumDialogPane::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const overrides( CLayoutPaneDialog )
{
	if ( IDW_CURR_IMAGE_PATH_LABEL == cmdId )
		rText = m_pAlbumView->GetImagePathKey().first.Get();
	else
		__super::QueryTooltipText( rText, cmdId, pTooltip );
}

bool CAlbumDialogPane::InputSlideDelay( ui::ComboField byField )
{
	UINT slideDelay;
	if ( !m_pSlideDelayCombo->InputValue( &slideDelay, byField, true ) )
		return false;		// invalid delay

	m_pAlbumView->SetSlideDelay( slideDelay );
	//m_pAlbumView->OnSlideDataChanged( false );		// don't set modified flag for this change
	OnSlideDelayChanged();			// update the content of combo box
	return true;
}

bool CAlbumDialogPane::SeekCurrentPos( int currIndex, bool forceLoad /*= false*/ )
{
	if ( !m_pAlbumView->IsValidIndex( currIndex ) )
		return false;

	if ( currIndex != m_pAlbumView->GetSlideData().GetCurrentIndex() || forceLoad )
	{
		m_pAlbumView->RefSlideData()->SetCurrentIndex( currIndex, true );
		m_pAlbumView->UpdateImage();		// this will also feedback on OnCurrPosChanged()
	}
	else if ( m_scrollSpin.GetPos() - 1 != currIndex )
		m_scrollSpin.SetPos( currIndex + 1 );

	return true;
}

void CAlbumDialogPane::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_pSlideDelayCombo->m_hWnd;

	m_pToolbar->DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDW_PLAY_DELAY_COMBO, *m_pSlideDelayCombo );
	DDX_Control( pDX, IDW_SEEK_CURR_POS_SPINEDIT, m_navPosEdit );
	DDX_Control( pDX, IDW_SCROLL_POS_SPIN, m_scrollSpin );
	DDX_Control( pDX, IDW_NAV_COUNT_LABEL, m_infoStatic );
	DDX_Control( pDX, IDW_CURR_IMAGE_PATH_LABEL, *m_pImagePathEdit );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_pToolbar->SetOwner( AfxGetMainWnd() );

			LOGFONT logFont;
			if ( GetFont()->GetLogFont( &logFont ) )
			{
				logFont.lfWeight = FW_BOLD;
				if ( m_boldFont.CreateFontIndirect( &logFont ) )
					m_pImagePathEdit->SetFont( &m_boldFont );
			}
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumDialogPane, CLayoutPaneDialog )
	ON_WM_CTLCOLOR()
	ON_WM_VSCROLL()
	ON_COMMAND( IDOK, OnOk )
	ON_COMMAND( ID_CM_ESCAPE_KEY, On_EscapeKey )
	ON_CBN_SELCHANGE( IDW_PLAY_DELAY_COMBO, OnCBnSelChange_SlideDelay )
	ON_CBN_CLOSEUP( IDW_PLAY_DELAY_COMBO, OnCBnCloseUp_SlideDelay )
	ON_CN_INPUTERROR( IDW_PLAY_DELAY_COMBO, OnCBnInputError_SlideDelay )
	ON_UPDATE_COMMAND_UI( IDW_PLAY_DELAY_COMBO, OnUpdate_SlideDelay )
	ON_EN_KILLFOCUS( IDW_SEEK_CURR_POS_SPINEDIT, OnEnKillFocus_SeekCurrPos )
END_MESSAGE_MAP()

HBRUSH CAlbumDialogPane::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor )
{
	HBRUSH hBrush = __super::OnCtlColor( pDC, pWnd, ctlColor );

	if ( CTLCOLOR_STATIC == ctlColor )
		if ( pWnd->m_hWnd == m_infoStatic.m_hWnd )
			pDC->SetTextColor( RGB( 0, 0, 255 ) );
		else if ( pWnd->m_hWnd == m_pImagePathEdit->m_hWnd )
			pDC->SetTextColor( RGB( 128, 0, 0 ) );

	return hBrush;
}

void CAlbumDialogPane::OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	__super::OnVScroll( sbCode, pos, pScrollBar );

	if ( pScrollBar->m_hWnd == m_scrollSpin.m_hWnd )
		switch ( sbCode )
		{
			case SB_THUMBPOSITION:
				m_pAlbumView->RefSlideData()->SetCurrentIndex( pos - 1 );
				OnCurrPosChanged();
				break;
			case SB_ENDSCROLL:
				m_pAlbumView->UpdateImage();		// this will also feedback on OnCurrPosChanged()
				break;
		}
}

void CAlbumDialogPane::OnOk( void )
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
	else if ( ui::OwnsFocus( m_navPosEdit ) )
	{
		if ( !SeekCurrentPos( m_scrollSpin.GetPos() - 1, true ) )
			ui::BeepSignal();
	}
}

void CAlbumDialogPane::On_EscapeKey( void )
{
	if ( ui::OwnsFocus( m_hWnd ) )
		m_pAlbumView->SetFocus();
}

void CAlbumDialogPane::OnCBnSelChange_SlideDelay( void )
{
	InputSlideDelay( ui::BySel );
}

void CAlbumDialogPane::OnCBnCloseUp_SlideDelay( void )
{
	m_pAlbumView->SetFocus();
}

void CAlbumDialogPane::OnCBnInputError_SlideDelay( void )
{
	OnSlideDelayChanged();
}

void CAlbumDialogPane::OnUpdate_SlideDelay( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( true );
}

void CAlbumDialogPane::OnEnKillFocus_SeekCurrPos( void )
{
	BOOL validInput;
	int currIndex = GetDlgItemInt( IDW_SEEK_CURR_POS_SPINEDIT, &validInput ) - 1;

	if ( !validInput || currIndex != m_pAlbumView->GetSlideData().GetCurrentIndex() )
		OnCurrPosChanged();
}
