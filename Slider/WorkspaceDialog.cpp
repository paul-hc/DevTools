
#include "stdafx.h"
#include "WorkspaceDialog.h"
#include "SlideData.h"				// af::ShowAlbumDialogBar
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/UI/Dialog_fwd.h"
#include "utl/UI/ImagingDirect2D.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWorkspaceDialog::CWorkspaceDialog( CWnd* pParent /*= NULL*/ )
	: CDialog( IDD_WORKSPACE_DIALOG, pParent )
	, m_data( CWorkspace::GetData() )
	, m_thumbnailerFlags( app::GetThumbnailer()->m_flags )
	, m_enlargeSmoothing( d2d::CDrawBitmapTraits::s_enlargeInterpolationMode != D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR )
{
	m_mruCountEdit.SetValidRange( Range< int >( 0, 16 ) );
	m_thumbListColCountEdit.SetValidRange( Range< int >( 1, 25 ) );
}

CWorkspaceDialog::~CWorkspaceDialog()
{
}

void CWorkspaceDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_mruCountEdit.m_hWnd;

	ui::DDX_Bool( pDX, CK_AUTOSAVE, m_data.m_autoSave );
	ui::DDX_Bool( pDX, IDC_ENLARGE_SMOOTHING_CHECK, m_enlargeSmoothing );

	ui::DDX_Flag( pDX, CK_PERSIST_OPEN_DOCS, m_data.m_wkspFlags, wf::PersistOpenDocs );
	ui::DDX_Flag( pDX, CK_PERSIST_ALBUM_IMAGE_STATE, m_data.m_wkspFlags, wf::PersistAlbumImageState );
	ui::DDX_Flag( pDX, ID_VIEW_ALBUMDIALOGBAR, m_data.m_albumViewFlags, af::ShowAlbumDialogBar );
	ui::DDX_Flag( pDX, IDC_SAVECUSTOMORDERUNDOREDO_CHECK, m_data.m_albumViewFlags, af::SaveCustomOrderUndoRedo );
	ui::DDX_Flag( pDX, IDC_PREFIX_DEEP_STREAM_NAMES_CHECK, m_data.m_wkspFlags, wf::PrefixDeepStreamNames );
	ui::DDX_Flag( pDX, IDC_ALLOW_EMBEDDED_FILE_TRANFERS_CHECK, m_data.m_wkspFlags, wf::AllowEmbeddedFileTransfers );
	ui::DDX_Flag( pDX, IDC_VISTA_STYLE_FILE_DLG_CHECK, m_data.m_wkspFlags, wf::UseVistaStyleFileDialog );

	ui::DDX_Flag( pDX, CK_SHOW_THUMB_VIEW, m_data.m_albumViewFlags, af::ShowThumbView );

	ui::DDX_Flag( pDX, IDC_AUTO_REGEN_SMALL_STG_THUMBS_CHECK, m_thumbnailerFlags, CThumbnailer::AutoRegenSmallStgThumbs );

	ui::DDX_EnumCombo( pDX, IDW_AUTO_IMAGE_SIZE_COMBO, m_autoImageSizeCombo, m_data.m_autoImageSize, ui::GetTags_AutoImageSize() );

	m_mruCountEdit.DDX_Number( pDX, m_data.m_mruCount, IDC_MAX_MRU_COUNT_EDIT );
	m_thumbListColCountEdit.DDX_Number( pDX, m_data.m_thumbListColCount, IDC_THUMB_COL_COUNT_EDIT );
	DDX_Control( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_thumbBoundsSizeCombo );

	if ( firstInit )
	{
		ui::WriteComboItems( m_thumbBoundsSizeCombo, thumb::GetTags_StdBoundsSize().GetUiTags() );
	}

	ui::DDX_Number( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_data.m_thumbBoundsSize );
	ui::DDV_NumberMinMax( pDX, IDC_THUMB_BOUNDS_SIZE_COMBO, m_data.m_thumbBoundsSize, thumb::MinBoundsSize, thumb::MaxBoundsSize );

	CDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CWorkspaceDialog, CDialog )
	ON_WM_DRAWITEM()
	ON_BN_CLICKED( CM_EDIT_BK_COLOR, CmEditBkColor )
	ON_BN_CLICKED( CM_SAVE_WORKSPACE, OnSaveAndClose )
	ON_BN_CLICKED( IDC_CLEAR_THUMB_CACHE_BUTTON, CmClearThumbCache )
	ON_BN_CLICKED( CM_EDIT_IMAGE_SEL_COLOR, CmEditImageSelColor )
	ON_BN_CLICKED( CM_EDIT_IMAGE_SEL_TEXT_COLOR, CmEditImageSelTextColor )
END_MESSAGE_MAP()

void CWorkspaceDialog::OnDrawItem( int ctlId, DRAWITEMSTRUCT* pDIS )
{
	switch ( ctlId )
	{
		case CM_EDIT_BK_COLOR:				FillRect( pDIS->hDC, &pDIS->rcItem, CBrush( m_data.m_defBkColor ) ); break;
		case CM_EDIT_IMAGE_SEL_COLOR:		FillRect( pDIS->hDC, &pDIS->rcItem, CBrush( m_data.GetImageSelColor() ) ); break;
		case CM_EDIT_IMAGE_SEL_TEXT_COLOR:	FillRect( pDIS->hDC, &pDIS->rcItem, CBrush( m_data.GetImageSelTextColor() ) ); break;
		default:
			CDialog::OnDrawItem( ctlId, pDIS );
	}
}

void CWorkspaceDialog::CmEditBkColor( void )
{
	CColorDialog colorDialog( m_data.m_defBkColor, CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR, this );
	if ( IDOK == colorDialog.DoModal() )
	{
		m_data.m_defBkColor = colorDialog.GetColor();
		GetDlgItem( CM_EDIT_BK_COLOR )->Invalidate();
	}
}

void CWorkspaceDialog::CmEditImageSelColor( void )
{
	if ( ui::IsKeyPressed( VK_SHIFT ) || ui::IsKeyPressed( VK_CONTROL ) )
		m_data.m_imageSelColor = color::Null;
	else
	{
		CColorDialog colorDialog( m_data.GetImageSelColor(), CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR, this );
		if ( IDOK == colorDialog.DoModal() )
			m_data.m_imageSelColor = colorDialog.GetColor();
	}
	GetDlgItem( CM_EDIT_IMAGE_SEL_COLOR )->Invalidate();
	GetDlgItem( CM_EDIT_IMAGE_SEL_TEXT_COLOR )->Invalidate();
}

void CWorkspaceDialog::CmEditImageSelTextColor( void )
{
	if ( ui::IsKeyPressed( VK_SHIFT ) || ui::IsKeyPressed( VK_CONTROL ) )
		m_data.m_imageSelTextColor = color::Null;
	else
	{
		CColorDialog colorDialog( m_data.GetImageSelTextColor(), CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR, this );
		if ( IDOK == colorDialog.DoModal() )
			m_data.m_imageSelTextColor = colorDialog.GetColor();
	}
	GetDlgItem( CM_EDIT_IMAGE_SEL_COLOR )->Invalidate();
	GetDlgItem( CM_EDIT_IMAGE_SEL_TEXT_COLOR )->Invalidate();
}

void CWorkspaceDialog::OnSaveAndClose( void )
{
	if ( UpdateData( TRUE ) )
		EndDialog( CM_SAVE_WORKSPACE );
}

void CWorkspaceDialog::CmClearThumbCache( void )
{
	if ( IDOK == app::GetUserReport().MessageBox( str::Format( _T("Clear the %d cached thumbnails in memory?"), app::GetThumbnailer()->GetCachedCount() ), MB_OKCANCEL | MB_ICONQUESTION ) )
		app::GetThumbnailer()->Clear();
}
