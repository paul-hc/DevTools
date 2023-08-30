
#include "pch.h"
#include "AlbumChildFrame.h"
#include "AlbumImageView.h"
#include "AlbumThumbListView.h"
#include "AlbumDoc.h"
#include "FileAttr.h"
#include "resource.h"
#include "utl/Range.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/UI/ControlBar_fwd.h"
#include "utl/UI/ToolbarButtons.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAlbumChildFrame implementation

IMPLEMENT_DYNCREATE( CAlbumChildFrame, CChildFrame )

CAlbumChildFrame::CAlbumChildFrame( void )
	: CChildFrame()
	, m_pAlbumToolBar( new mfc::CFixedToolBar() )
	, m_pThumbsListView( nullptr )
	, m_pAlbumImageView( nullptr )
	, m_doneInit( false )
{
	m_bEnableFloatingBars = TRUE;
	GetDockingManager()->DisableRestoreDockState( TRUE );		// to disable loading of docking layout from the Registry
}

CAlbumChildFrame::~CAlbumChildFrame()
{
}

IImageView* CAlbumChildFrame::GetImageView( void ) const override
{
	return m_pAlbumImageView;
}

void CAlbumChildFrame::ShowBar( bool show ) implement
{
	m_pAlbumToolBar->ShowBar( show );
}

void CAlbumChildFrame::OnNavRangeChanged( void ) implement
{	// 1-based displayed index
	int imageCount = static_cast<int>( m_pAlbumImageView->GetDocument()->GetImageCount() );

	CMFCToolBarSpinEditBoxButton* pSpinEdit = mfc::ToolBar_LookupButton<CMFCToolBarSpinEditBoxButton>( *m_pAlbumToolBar, IDW_SEEK_CURR_POS_SPINEDIT );
	pSpinEdit->SetRange( 1, imageCount );

	static const std::tstring prefix = _T("of ");
	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), IDW_NAV_COUNT_LABEL, prefix + num::FormatNumber(imageCount, str::GetUserLocale()));
}

void CAlbumChildFrame::OnCurrPosChanged( void ) implement
{
	int currIndex = m_pAlbumImageView->GetSlideData().GetCurrentIndex();
	bool valid = m_pAlbumImageView->IsValidIndex( currIndex );
	std::tstring imageFilePath;

	if ( valid )
		if ( const CFileAttr* pFileAttr = m_pAlbumImageView->GetDocument()->GetModel()->GetFileAttr( currIndex ) )
			imageFilePath = pFileAttr->GetPath().FormatPretty();

	CMFCToolBarSpinEditBoxButton* pSpinEdit = mfc::ToolBar_LookupButton<CMFCToolBarSpinEditBoxButton>( *m_pAlbumToolBar, IDW_SEEK_CURR_POS_SPINEDIT );
	CSpinButtonCtrl* pSpinCtrl = pSpinEdit->GetSpinControl();

	pSpinEdit->EnableWindow( valid );

	if ( valid )
	{	// 1-based displayed index
		if ( pSpinCtrl->GetPos() - 1 != currIndex )
			pSpinCtrl->SetPos( currIndex + 1 );
	}
	else
		pSpinEdit->SetContents( _T("") );

	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), IDW_CURR_IMAGE_PATH_LABEL, imageFilePath );
	m_pAlbumToolBar->AdjustLayout();
}

void CAlbumChildFrame::OnSlideDelayChanged( void ) implement
{
	mfc::CStockValuesComboBoxButton* pPlayDelayCombo = mfc::ToolBar_LookupButton<mfc::CStockValuesComboBoxButton>( *m_pAlbumToolBar, IDW_PLAY_DELAY_COMBO );
	double slideDelaySecs = ui::CDurationSecondsStockTags::FromMiliseconds( m_pAlbumImageView->GetSlideData().m_slideDelay );

	pPlayDelayCombo->OutputValue( slideDelaySecs );
}

bool CAlbumChildFrame::InputSlideDelay( ui::ComboField byField )
{
	const mfc::CStockValuesComboBoxButton* pPlayDelayCombo = mfc::ToolBar_LookupButton<mfc::CStockValuesComboBoxButton>( *m_pAlbumToolBar, IDW_PLAY_DELAY_COMBO );
	double slideDelaySecs;

	ASSERT_PTR( pPlayDelayCombo );
	if ( !pPlayDelayCombo->InputValue( &slideDelaySecs, byField, true ) )
		return false;

	m_pAlbumImageView->SetSlideDelay( ui::CDurationSecondsStockTags::ToMiliseconds( slideDelaySecs ) );
	OnSlideDelayChanged();			// update the content of combo box
	return true;
}

bool CAlbumChildFrame::InputCurrentPos( void )
{
	CSpinButtonCtrl* pSpinCtrl = mfc::ToolBar_LookupButton<CMFCToolBarSpinEditBoxButton>( *m_pAlbumToolBar, IDW_SEEK_CURR_POS_SPINEDIT )->GetSpinControl();
	int currIndex = pSpinCtrl->GetPos() - 1;

	if ( !m_pAlbumImageView->IsValidIndex( currIndex ) )
		return false;

	m_pAlbumImageView->PtrSlideData()->SetCurrentIndex( currIndex, true );
	m_pAlbumImageView->UpdateImage();		// this will also feedback on OnCurrPosChanged()

	return true;
}

void CAlbumChildFrame::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;

	if ( IDW_CURR_IMAGE_PATH_LABEL == cmdId )
		rText = mfc::ToolBar_LookupButton<mfc::CLabelButton>( *m_pAlbumToolBar, IDW_CURR_IMAGE_PATH_LABEL )->m_strText.GetString();
}

void CAlbumChildFrame::BuildAlbumToolbar( void )
{
	m_pAlbumToolBar->SetWindowText( _T("Album") );
	m_pAlbumToolBar->RemoveAllButtons();

	m_pAlbumToolBar->InsertButton( new CMFCToolBarButton( ID_EDIT_ALBUM, -1 ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new mfc::CStockValuesComboBoxButton( IDW_PLAY_DELAY_COMBO, ui::CDurationSecondsStockTags::Instance(), DurationComboWidth ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new CMFCToolBarButton( IDW_AUTO_SEEK_IMAGE_POS_CHECK, -1 ) );
	m_pAlbumToolBar->InsertButton( new CMFCToolBarSpinEditBoxButton( IDW_SEEK_CURR_POS_SPINEDIT, -1, ES_NUMBER | ES_AUTOHSCROLL, SeekCurrPosSpinEditWidth ) );
	m_pAlbumToolBar->InsertButton( new mfc::CLabelButton( IDW_NAV_COUNT_LABEL ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new mfc::CLabelButton( IDW_CURR_IMAGE_PATH_LABEL, mfc::BO_BoldFont | mfc::BO_QueryToolTip ) );

	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), ID_EDIT_ALBUM, _T("Edit Album") );
	m_pAlbumToolBar->RecalcLayout();
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumChildFrame, CChildFrame )
	ON_WM_CREATE()
	ON_COMMAND( ID_VIEW_ALBUM_DIALOG_BAR, OnToggle_ViewAlbumPane )
	ON_UPDATE_COMMAND_UI( ID_VIEW_ALBUM_DIALOG_BAR, OnUpdate_ViewAlbumPane )	// CFrameWnd::OnUpdateControlBarMenu
	ON_BN_CLICKED( IDW_PLAY_DELAY_COMBO, OnEditInput_PlayDelayCombo )
	ON_CBN_SELENDOK( IDW_PLAY_DELAY_COMBO, OnCBnSelChange_PlayDelayCombo )
	ON_UPDATE_COMMAND_UI( IDW_PLAY_DELAY_COMBO, OnUpdateAlways )
	ON_COMMAND( IDW_AUTO_SEEK_IMAGE_POS_CHECK, OnToggle_AutoSeekImagePos )
	ON_UPDATE_COMMAND_UI( IDW_AUTO_SEEK_IMAGE_POS_CHECK, OnUpdate_AutoSeekImagePos )
	ON_EN_UPDATE( IDW_SEEK_CURR_POS_SPINEDIT, OnEnUpdate_SeekCurrPosSpinEdit )
	ON_COMMAND( IDW_SEEK_CURR_POS_SPINEDIT, On_SeekCurrPosSpinEdit )			// called when ENTER is pressed
	ON_UPDATE_COMMAND_UI( IDW_SEEK_CURR_POS_SPINEDIT, OnUpdateAlways )
	ON_COMMAND( IDW_CURR_IMAGE_PATH_LABEL, On_CopyCurrImagePath )
	ON_UPDATE_COMMAND_UI( IDW_CURR_IMAGE_PATH_LABEL, OnUpdateAlways )
END_MESSAGE_MAP()

int CAlbumChildFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == CChildFrame::OnCreate( pCS ) )
		return -1;

	if ( !m_pAlbumToolBar->Create( this, mfc::CFixedToolBar::Style, ID_VIEW_ALBUM_DIALOG_BAR ) ||
		 !m_pAlbumToolBar->LoadToolBar( IDR_STD_STATUS_STRIP ) )					// placeholder toolbar resource - required to display the replacement button
		return -1;

	BuildAlbumToolbar();

	DockPane( m_pAlbumToolBar.get() );		// toolbar will stretch horizontally
	//m_pAlbumToolBar->DockToFrameWindow( CBRS_ALIGN_TOP );		// make the dialog pane stretchable horizontally

	m_doneInit = true;		// can start processing notifications
	// note: it's too early to update pane visibility; we have to delay to after loading CSlideData data-memberby CAlbumDoc, in CAlbumImageView::UpdateChildBarsState()
	return 0;
}

BOOL CAlbumChildFrame::OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext ) overrides(CChildFrame)
{
	pCS;
	VERIFY( m_splitterWnd.CreateStatic( this, 1, 2, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST ) );

	CRect thumbViewRect = CAlbumThumbListView::GetListWindowRect();

	m_splitterWnd.SetColumnInfo( ThumbView, thumbViewRect.Width(), 0 );
	VERIFY( m_splitterWnd.CreateView( 0, ThumbView, RUNTIME_CLASS( CAlbumThumbListView ), CSize( thumbViewRect.Width(), 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_splitterWnd.SetColumnInfo( PictureView, 0, 30 );
	VERIFY( m_splitterWnd.CreateView( 0, PictureView, RUNTIME_CLASS( CAlbumImageView ), CSize( 0, 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_pThumbsListView = checked_static_cast<CAlbumThumbListView*>( m_splitterWnd.GetPane( 0, ThumbView ) );
	m_pAlbumImageView = checked_static_cast<CAlbumImageView*>( m_splitterWnd.GetPane( 0, PictureView ) );
	StoreImageView( m_pAlbumImageView );

	m_pThumbsListView->StorePeerView( m_pAlbumImageView );
	m_pAlbumImageView->StorePeerView( m_pThumbsListView, this );
	return TRUE;
}

void CAlbumChildFrame::OnToggle_ViewAlbumPane( void )
{	// toggle the visibility of the album dialog bar
	REQUIRE( GetAlbumImageView()->GetSlideData().HasShowFlag( af::ShowAlbumDialogBar ) == m_pAlbumToolBar->IsBarVisible() );	// consistent?

	bool show = GetAlbumImageView()->PtrSlideData()->ToggleShowFlag( af::ShowAlbumDialogBar );		// toggle visibility flag

	m_pAlbumToolBar->ShowBar( show );
}

void CAlbumChildFrame::OnUpdate_ViewAlbumPane( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
	pCmdUI->SetCheck( m_pAlbumToolBar->IsBarVisible() );
}

void CAlbumChildFrame::OnEditInput_PlayDelayCombo( void )
{
	if ( InputSlideDelay( ui::ByEdit ) )
		m_pAlbumImageView->SetFocus();
	else
		ui::BeepSignal();
}

void CAlbumChildFrame::OnCBnSelChange_PlayDelayCombo( void )
{
	if ( InputSlideDelay( ui::BySel ) )
		m_pAlbumImageView->SetFocus();
}

void CAlbumChildFrame::OnToggle_AutoSeekImagePos( void )
{
	if ( m_pAlbumImageView->PtrSlideData()->ToggleShowFlag( af::AutoSeekAlbumImagePos ) )
		InputCurrentPos();
}

void CAlbumChildFrame::OnUpdate_AutoSeekImagePos( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_pAlbumImageView->GetSlideData().HasShowFlag( af::AutoSeekAlbumImagePos ) );
}

void CAlbumChildFrame::OnEnUpdate_SeekCurrPosSpinEdit( void )
{
	if ( m_doneInit )
		if ( m_pAlbumImageView->GetSlideData().HasShowFlag( af::AutoSeekAlbumImagePos ) )
			On_SeekCurrPosSpinEdit();
}

void CAlbumChildFrame::On_SeekCurrPosSpinEdit( void )
{
	if ( !InputCurrentPos() )
		ui::BeepSignal();
}

void CAlbumChildFrame::On_CopyCurrImagePath( void )
{
	CTextClipboard::CopyText( mfc::ToolBar_LookupButton<mfc::CLabelButton>( *m_pAlbumToolBar, IDW_CURR_IMAGE_PATH_LABEL )->m_strText.GetString(), m_hWnd );
}

void CAlbumChildFrame::OnUpdateAlways( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
}
