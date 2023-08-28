
#include "pch.h"
#include "WorkspaceDialog.h"
#include "Album_fwd.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/UI/ColorPickerButton.h"
#include "utl/UI/Direct2D.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/StockValuesComboBox.hxx"


CWorkspaceDialog::CWorkspaceDialog( CWnd* pParent /*= nullptr*/ )
	: CLayoutDialog( IDD_WORKSPACE_DIALOG, pParent )
	, m_data( CWorkspace::GetData() )
	, m_thumbnailerFlags( app::GetThumbnailer()->m_flags )
	, m_smoothingMode( d2d::CSharedTraits::Instance().IsSmoothingMode() )
	, m_defaultSlideDelay( CWorkspace::Instance().GetDefaultSlideDelay() )
	, m_pDefBkColorPicker( new CColorPickerButton() )
	, m_pImageSelColorPicker( new CColorPickerButton() )
	, m_pImageSelTextColorPicker( new CColorPickerButton() )
{
	m_mruCountEdit.SetValidRange( Range<int>( 0, 16 ) );
	m_thumbListColumnCountEdit.SetValidRange( Range<int>( 1, 25 ) );
}

CWorkspaceDialog::~CWorkspaceDialog()
{
}

void CWorkspaceDialog::EnableCtrls( void )
{
	// enable
	const UINT ctrlIds[] = { CM_EDIT_IMAGE_SEL_COLOR, IDC_EDIT_IMAGE_SEL_COLOR_LABEL, CM_EDIT_IMAGE_SEL_TEXT_COLOR, IDC_EDIT_IMAGE_SEL_TEXT_COLOR_LABEL };

	ui::EnableControls( m_hWnd, ARRAY_SPAN( ctrlIds ), !HasFlag( m_data.m_wkspFlags, wf::UseThemedThumbListDraw ) );	// disable non-themed color editing
}

void CWorkspaceDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_mruCountEdit.m_hWnd;

	ui::DDX_Bool( pDX, IDC_AUTOSAVE_CHECK, m_data.m_autoSave );
	ui::DDX_Bool( pDX, IDC_ENLARGE_SMOOTHING_CHECK, m_smoothingMode );

	ui::DDX_Flag( pDX, IDC_PERSIST_OPEN_DOCS_CHECK, m_data.m_wkspFlags, wf::PersistOpenDocs );
	ui::DDX_Flag( pDX, IDC_PERSIST_ALBUM_IMAGE_STATE_CHECK, m_data.m_wkspFlags, wf::PersistAlbumImageState );
	ui::DDX_Flag( pDX, ID_VIEW_ALBUM_DIALOG_BAR, m_data.m_albumViewFlags, af::ShowAlbumDialogBar );
	ui::DDX_Flag( pDX, IDC_SAVECUSTOMORDERUNDOREDO_CHECK, m_data.m_albumViewFlags, af::SaveCustomOrderUndoRedo );
	ui::DDX_Flag( pDX, IDC_PREFIX_DEEP_STREAM_NAMES_CHECK, m_data.m_wkspFlags, wf::DeepStreamPaths );
	ui::DDX_Flag( pDX, IDC_ALLOW_EMBEDDED_FILE_TRANFERS_CHECK, m_data.m_wkspFlags, wf::AllowEmbeddedFileTransfers );
	ui::DDX_Flag( pDX, IDC_VISTA_STYLE_FILE_DLG_CHECK, m_data.m_wkspFlags, wf::UseVistaStyleFileDialog );
	ui::DDX_Flag( pDX, IDC_USE_THEMED_THUMB_LIST_DRAW_CHECK, m_data.m_wkspFlags, wf::UseThemedThumbListDraw );

	ui::DDX_ColorEditor( pDX, ID_EDIT_BK_COLOR, m_pDefBkColorPicker.get(), &m_data.m_defBkColor );
	ui::DDX_ColorEditor( pDX, CM_EDIT_IMAGE_SEL_COLOR, m_pImageSelColorPicker.get(), &m_data.m_imageSelColor );
	ui::DDX_ColorEditor( pDX, CM_EDIT_IMAGE_SEL_TEXT_COLOR, m_pImageSelTextColorPicker.get(), &m_data.m_imageSelTextColor );

	ui::DDX_Flag( pDX, CK_SHOW_THUMB_VIEW, m_data.m_albumViewFlags, af::ShowThumbView );
	ui::DDX_Flag( pDX, IDC_AUTO_REGEN_SMALL_STG_THUMBS_CHECK, m_thumbnailerFlags, CThumbnailer::AutoRegenSmallStgThumbs );
	ui::DDX_EnumCombo( pDX, IDW_IMAGE_SCALING_COMBO, m_imageScalingCombo, m_data.m_scalingMode, ui::GetTags_ImageScalingMode() );

	m_mruCountEdit.DDX_Number( pDX, m_data.m_mruCount, IDC_MAX_MRU_COUNT_EDIT );
	m_thumbListColumnCountEdit.DDX_Number( pDX, m_data.m_thumbListColumnCount, IDC_THUMB_COL_COUNT_EDIT );
	DDX_Control( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_thumbBoundsSizeCombo );

	if ( firstInit )
	{
		ui::WriteComboItems( m_thumbBoundsSizeCombo, thumb::GetTags_StdBoundsSize().GetUiTags() );
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		EnableCtrls();

	ui::DDX_Number( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_data.m_thumbBoundsSize );
	ui::DDV_NumberMinMax( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_data.m_thumbBoundsSize, thumb::MinBoundsSize, thumb::MaxBoundsSize );

	m_slideDelayCombo.DDX_Value( pDX, m_defaultSlideDelay, IDC_DEFAULT_SLIDE_DELAY_COMBO );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CWorkspaceDialog, CLayoutDialog )
	ON_BN_CLICKED( ID_EDIT_BK_COLOR, On_EditBkColor )
	ON_BN_CLICKED( CM_SAVE_WORKSPACE, OnSaveAndClose )
	ON_BN_CLICKED( IDC_USE_THEMED_THUMB_LIST_DRAW_CHECK, OnToggle_UseThemedThumbListDraw )
	ON_BN_CLICKED( IDC_CLEAR_THUMB_CACHE_BUTTON, CmClearThumbCache )
	ON_BN_CLICKED( CM_EDIT_IMAGE_SEL_COLOR, CmEditImageSelColor )
	ON_BN_CLICKED( CM_EDIT_IMAGE_SEL_TEXT_COLOR, CmEditImageSelTextColor )
END_MESSAGE_MAP()

void CWorkspaceDialog::On_EditBkColor( void )
{
	m_data.m_defBkColor.Set( m_pDefBkColorPicker->GetColor() );
}

void CWorkspaceDialog::CmEditImageSelColor( void )
{
	m_data.m_imageSelColor.Set( m_pImageSelColorPicker->GetColor() );
}

void CWorkspaceDialog::CmEditImageSelTextColor( void )
{
	m_data.m_imageSelTextColor.Set( m_pImageSelTextColorPicker->GetColor() );
}

void CWorkspaceDialog::OnSaveAndClose( void )
{
	if ( UpdateData( DialogSaveChanges ) )
		EndDialog( CM_SAVE_WORKSPACE );
}

void CWorkspaceDialog::OnToggle_UseThemedThumbListDraw( void )
{
	SetFlag( m_data.m_wkspFlags, wf::UseThemedThumbListDraw, IsDlgButtonChecked( IDC_USE_THEMED_THUMB_LIST_DRAW_CHECK ) != FALSE );
	EnableCtrls();
	app::GetApp()->UpdateAllViews( Hint_ViewUpdate );
}

void CWorkspaceDialog::CmClearThumbCache( void )
{
	if ( IDOK == app::GetUserReport().MessageBox( str::Format( _T("Clear the %d cached thumbnails in memory?"), app::GetThumbnailer()->GetCachedCount() ), MB_OKCANCEL | MB_ICONQUESTION ) )
		app::GetThumbnailer()->Clear();
}
