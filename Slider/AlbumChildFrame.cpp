
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
#include "utl/UI/DataAdapters.h"
#include "utl/UI/ToolbarButtons.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	class CDurationSecondsStockTags : public ui::CStockTags<double>
	{
		CDurationSecondsStockTags( void );
	public:
		static const CDurationSecondsStockTags* Instance( void );

		static double FromMiliseconds( UINT miliseconds ) { return static_cast<double>( miliseconds ) / 1000.0; }
		static UINT ToMiliseconds( double seconds ) { return static_cast<UINT>( seconds * 1000.0 ); }
	public:
		static const ui::CNumericUnitAdapter<double> s_secondsAdapter;
	};


	// CDurationSecondsStockTags implementation

	const ui::CNumericUnitAdapter<double> CDurationSecondsStockTags::s_secondsAdapter( _T(" sec") );

	CDurationSecondsStockTags::CDurationSecondsStockTags( void )
		: CStockTags<double>( &s_secondsAdapter, _T("0.1|0.25|0.5|0.75|1|1.5|2|3|4|5|8|10|12|15|17|20|25|30") )
	{
		Range<double> limits( FromMiliseconds( USER_TIMER_MINIMUM ), FromMiliseconds( USER_TIMER_MAXIMUM ) );

		SetLimits( limits, ui::LimitRange );
	}

	const CDurationSecondsStockTags* CDurationSecondsStockTags::Instance( void )
	{
		static const CDurationSecondsStockTags s_stockTags;
		return &s_stockTags;
	}
}


static const bool s_newBar = true;

// CAlbumChildFrame implementation

IMPLEMENT_DYNCREATE( CAlbumChildFrame, CChildFrame )

CAlbumChildFrame::CAlbumChildFrame( void )
	: CChildFrame()
	, m_pAlbumToolBar( new mfc::CFixedToolBar() )
	, m_pThumbsListView( nullptr )
	, m_pAlbumImageView( nullptr )
	, m_pAlbumView( nullptr )
{
	GetDockingManager()->DisableRestoreDockState( TRUE );		// to disable loading of docking layout from the Registry
	if ( s_newBar )
		m_bEnableFloatingBars = TRUE;
}

CAlbumChildFrame::~CAlbumChildFrame()
{
}

IImageView* CAlbumChildFrame::GetImageView( void ) const
{
	return m_pAlbumImageView;
}

void CAlbumChildFrame::InitAlbumImageView( CAlbumImageView* pAlbumView ) implement
{
	m_pAlbumView = pAlbumView;

	if ( s_newBar )
		m_albumDlgPane.InitAlbumImageView( pAlbumView );
}

void CAlbumChildFrame::ShowBar( bool show ) implement
{
	s_newBar ? m_pAlbumToolBar->ShowPane( show, false, false ) : m_albumDlgPane.ShowPane( show, false, false );
}

void CAlbumChildFrame::OnNavRangeChanged( void ) implement
{	// 1-based displayed index
	if ( nullptr == m_pAlbumView )
		return;		// not yet initialized

	int imageCount = static_cast<int>( m_pAlbumView->GetDocument()->GetImageCount() );

	CMFCToolBarSpinEditBoxButton* pSpinEdit = mfc::ToolBar_LookupButton<CMFCToolBarSpinEditBoxButton>( *m_pAlbumToolBar, IDW_SEEK_CURR_POS_SPINEDIT );
	pSpinEdit->SetRange( 1, imageCount );

	static const std::tstring prefix = _T("of ");
	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), IDW_NAV_COUNT_LABEL, prefix + num::FormatNumber(imageCount, str::GetUserLocale()));

	if ( s_newBar )
		m_albumDlgPane.OnNavRangeChanged();
}

void CAlbumChildFrame::OnCurrPosChanged( void ) implement
{
	if ( nullptr == m_pAlbumView )
		return;		// not yet initialized

	int currIndex = m_pAlbumView->GetSlideData().GetCurrentIndex();
	bool valid = m_pAlbumView->IsValidIndex( currIndex );
	std::tstring imageFilePath;

	if ( valid )
		if ( const CFileAttr* pFileAttr = m_pAlbumView->GetDocument()->GetModel()->GetFileAttr( currIndex ) )
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

	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), IDW_CURR_IMAGE_PATH_LABEL, imageFilePath);
	m_pAlbumToolBar->AdjustLayout();

	if ( s_newBar )
		m_albumDlgPane.OnCurrPosChanged();
}

void CAlbumChildFrame::OnSlideDelayChanged( void ) implement
{
	mfc::CStockValuesComboBoxButton* pPlayDelayCombo = mfc::ToolBar_LookupButton<mfc::CStockValuesComboBoxButton>( *m_pAlbumToolBar, IDW_PLAY_DELAY_COMBO );
	double slideDelaySecs = app::CDurationSecondsStockTags::FromMiliseconds( m_pAlbumView->GetSlideData().m_slideDelay );

	pPlayDelayCombo->OutputValue( slideDelaySecs );

	if ( s_newBar )
		m_albumDlgPane.OnSlideDelayChanged();
}

bool CAlbumChildFrame::InputSlideDelay( ui::ComboField byField )
{
	const mfc::CStockValuesComboBoxButton* pPlayDelayCombo = mfc::ToolBar_LookupButton<mfc::CStockValuesComboBoxButton>( *m_pAlbumToolBar, IDW_PLAY_DELAY_COMBO );
	double slideDelaySecs;

	ASSERT_PTR( pPlayDelayCombo );
	if ( !pPlayDelayCombo->InputValue( &slideDelaySecs, byField, true ) )
		return false;

	m_pAlbumView->SetSlideDelay( app::CDurationSecondsStockTags::ToMiliseconds( slideDelaySecs ) );
	OnSlideDelayChanged();			// update the content of combo box
	return true;
}

bool CAlbumChildFrame::InputCurrentPos( void )
{
	CSpinButtonCtrl* pSpinCtrl = mfc::ToolBar_LookupButton<CMFCToolBarSpinEditBoxButton>( *m_pAlbumToolBar, IDW_SEEK_CURR_POS_SPINEDIT )->GetSpinControl();
	int currIndex = pSpinCtrl->GetPos() - 1;

	if ( !m_pAlbumView->IsValidIndex( currIndex ) )
		return false;

	m_pAlbumView->RefSlideData()->SetCurrentIndex( currIndex, true );
	m_pAlbumView->UpdateImage();		// this will also feedback on OnCurrPosChanged()

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
	VERIFY( m_pAlbumToolBar->LoadToolBar( ID_VIEW_ALBUM_DIALOG_BAR ) );	// just one blank button placeholder - required to display the replacement button
	m_pAlbumToolBar->SetWindowText( _T("Album") );
	m_pAlbumToolBar->RemoveAllButtons();

	m_pAlbumToolBar->InsertButton( new CMFCToolBarButton( ID_EDIT_ALBUM, mfc::FindButtonImageIndex( ID_EDIT_ITEM ) ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new mfc::CStockValuesComboBoxButton( IDW_PLAY_DELAY_COMBO, app::CDurationSecondsStockTags::Instance(), DurationComboWidth ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new CMFCToolBarSpinEditBoxButton( IDW_SEEK_CURR_POS_SPINEDIT, -1, ES_NUMBER | ES_AUTOHSCROLL, SeekCurrPosSpinEditWidth ) );
	m_pAlbumToolBar->InsertButton( new mfc::CLabelButton( IDW_NAV_COUNT_LABEL ) );
	m_pAlbumToolBar->InsertSeparator();
	m_pAlbumToolBar->InsertButton( new mfc::CLabelButton( IDW_CURR_IMAGE_PATH_LABEL, mfc::BO_StretchWidth | mfc::BO_BoldFont | mfc::BO_QueryToolTip ) );
	m_pAlbumToolBar->InsertButton( new CMFCToolBarButton( ID_COPY_CURR_IMAGE_PATH, mfc::FindButtonImageIndex( ID_EDIT_COPY ) ) );

	mfc::ToolBar_SetBtnText( m_pAlbumToolBar.get(), ID_EDIT_ALBUM, _T("Edit Album"));
	m_pAlbumToolBar->RecalcLayout();
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumChildFrame, CChildFrame )
	ON_WM_CREATE()
	ON_COMMAND( ID_VIEW_ALBUM_DIALOG_BAR, OnToggle_ViewAlbumPane )
	ON_UPDATE_COMMAND_UI( ID_VIEW_ALBUM_DIALOG_BAR, OnUpdate_ViewAlbumPane )		// CFrameWnd::OnUpdateControlBarMenu
	ON_BN_CLICKED( IDW_PLAY_DELAY_COMBO, OnEditInput_PlayDelayCombo )
	ON_CBN_SELENDOK( IDW_PLAY_DELAY_COMBO, OnCBnSelChange_PlayDelayCombo )
	ON_UPDATE_COMMAND_UI( IDW_PLAY_DELAY_COMBO, OnUpdateAlways )
	ON_COMMAND( IDW_SEEK_CURR_POS_SPINEDIT, OnEnChange_SeekCurrPosSpinEdit )		// covers for EN_CHANGE
	ON_UPDATE_COMMAND_UI( IDW_SEEK_CURR_POS_SPINEDIT, OnUpdateAlways )
	ON_COMMAND( IDW_CURR_IMAGE_PATH_LABEL, On_CopyCurrImagePath )
	ON_UPDATE_COMMAND_UI( IDW_CURR_IMAGE_PATH_LABEL, OnUpdateAlways )
	ON_COMMAND( ID_COPY_CURR_IMAGE_PATH, On_CopyCurrImagePath )
	ON_UPDATE_COMMAND_UI( ID_COPY_CURR_IMAGE_PATH, OnUpdateAlways )
END_MESSAGE_MAP()

int CAlbumChildFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == CChildFrame::OnCreate( pCS ) )
		return -1;

	if ( s_newBar )
	{
		if ( m_pAlbumToolBar->Create( this, mfc::CFixedToolBar::Style, ID_VIEW_ALBUM_DIALOG_BAR ) )
			BuildAlbumToolbar();
		else
			return -1;

		DockPane( m_pAlbumToolBar.get() );		// toolbar will stretch horizontally
		//m_pAlbumToolBar->DockToFrameWindow( CBRS_ALIGN_TOP );		// make the dialog pane stretchable horizontally
	}

	// initialize album dialog-bar
	enum { PaneStyle = WS_VISIBLE | WS_CHILD | CBRS_FLOAT_MULTI | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED /*| CBRS_BOTTOM*/ };

	VERIFY( m_albumDlgPane.Create( _T("Album Dialog Bar (old)"), this, false, IDD_ALBUMDIALOGBAR, PaneStyle, ID_VIEW_ALBUM_DIALOG_BAR ) );

	if ( !s_newBar )
		m_albumDlgPane.DockToFrameWindow( CBRS_ALIGN_BOTTOM );		// make the dialog pane stretchable horizontally
	else
	{
		CSize minSize;
		m_albumDlgPane.GetMinSize( minSize );
		minSize.cy += 30;		// floating frame height

		CRect screenRect;
		GetWindowRect( &screenRect );
		screenRect.OffsetRect( 16, screenRect.Height() + 10 );
		ui::SetRectSize( screenRect, minSize );

		m_albumDlgPane.FloatPane( screenRect );
		m_albumDlgPane.ShowBar( true );
	}

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
	m_pAlbumImageView->StorePeerView( m_pThumbsListView, s_newBar ? static_cast<IAlbumBar*>( this ) : static_cast<IAlbumBar*>( &m_albumDlgPane ) );
	return TRUE;
}

void CAlbumChildFrame::OnToggle_ViewAlbumPane( void )
{	// toggle the visibility of the album dialog bar
	REQUIRE( (BOOL)GetAlbumImageView()->GetSlideData().HasShowFlag( af::ShowAlbumDialogBar ) == m_albumDlgPane.IsVisible() );	// consistent?

	GetAlbumImageView()->RefSlideData()->ToggleShowFlag( af::ShowAlbumDialogBar );		// toggle visibility flag
	bool show = GetAlbumImageView()->GetSlideData().HasShowFlag( af::ShowAlbumDialogBar );

	ShowPane( &m_albumDlgPane, show, false, false );
}

void CAlbumChildFrame::OnUpdate_ViewAlbumPane( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
	pCmdUI->SetCheck( m_albumDlgPane.IsVisible() );
}

void CAlbumChildFrame::OnUpdateAlways( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
}

void CAlbumChildFrame::OnEnChange_SeekCurrPosSpinEdit( void )
{
	if ( !InputCurrentPos() )
		ui::BeepSignal();
}

void CAlbumChildFrame::OnEditInput_PlayDelayCombo( void )
{
	if ( InputSlideDelay( ui::ByEdit ) )
		m_pAlbumView->SetFocus();
	else
		ui::BeepSignal();
}

void CAlbumChildFrame::OnCBnSelChange_PlayDelayCombo( void )
{
	if ( InputSlideDelay( ui::BySel ) )
		m_pAlbumView->SetFocus();
}

void CAlbumChildFrame::On_CopyCurrImagePath( void )
{
	CTextClipboard::CopyText( mfc::ToolBar_LookupButton<mfc::CLabelButton>( *m_pAlbumToolBar, IDW_CURR_IMAGE_PATH_LABEL )->m_strText.GetString(), m_hWnd );
}
