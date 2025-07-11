
#include "pch.h"
#include "ImageDialog.h"
#include "Application.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Color.h"
#include "utl/UI/ColorPickerButton.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/Imaging.h"
#include "utl/UI/ImagingGdiPlus.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Pixel.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dlg[] = _T("ImageDialog");
	static const TCHAR entry_sampleMode[] = _T("SampleMode");
	static const TCHAR entry_imagingApi[] = _T("ImagingApi");
	static const TCHAR entry_framePos[] = _T("FramePos");
	static const TCHAR entry_forceResolution[] = _T("ForceResolution");
	static const TCHAR entry_showFlags[] = _T("ShowFlags");
	static const TCHAR entry_zoom[] = _T("Zoom");
	static const TCHAR entry_imagePathHistory[] = _T("ImagePathHistory");
	static const TCHAR entry_imagePath[] = _T("ImagePath");
	static const TCHAR entry_bkColor[] = _T("BkColor");
	//static const TCHAR entry_bkColorHistory[] = _T("BkColorHistory");
	static const TCHAR entry_stacking[] = _T("Stacking");
	static const TCHAR entry_spacing[] = _T("Spacing");
	static const TCHAR entry_statusAlpha[] = _T("StatusAlpha");
	static const TCHAR entry_convertFlags[] = _T("ConvertFlags");
	static const TCHAR entry_sourceAlpha[] = _T("SourceAlpha");
	static const TCHAR entry_pixelAlpha[] = _T("PixelAlpha");
	static const TCHAR entry_disabledAlpha[] = _T("DisabledAlpha");
	static const TCHAR entry_alphaPageKeepEqual[] = _T("AlphaPageKeepEqual");
	static const TCHAR entry_contrastPct[] = _T("contrastPercentage");
}


static const TCHAR bkColorSet[] = _T("#FFFFFF (White);#FFE5B4 (Peach);#ACE1AF (Celadon);#D1E231 (Pear);#F5DEB3 (Wheat);#C19A6B (Lion);#F4A460 (Sandy Brown)");

enum { ImagePct = 80, ColorBoardPct = 100 - ImagePct };

namespace layout
{
	#define Lay_SizeImageY pctSizeY( ImagePct )
	#define Lay_MoveTableY pctMoveY( ImagePct )
	#define Lay_SizeTableY pctSizeY( ColorBoardPct )

	static CDualLayoutStyle s_dualStyles[] =
	{
		{ IDC_IMAGE_PATH_COMBO, SizeX, UseExpanded },
		{ IDC_SAMPLE_MODE_SHEET, SizeX, UseExpanded },
		{ IDC_IMAGE_INFO_STATIC, SizeX, UseExpanded },
		{ IDC_TRANSP_COLOR_SAMPLE, MoveX, UseExpanded },
		{ ID_RESET_TRANSP_COLOR, MoveX, UseExpanded },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX, UseExpanded },

		{ IDC_IMAGE_SAMPLE, SizeX | Lay_SizeImageY, SizeX | SizeY },
		{ IDC_PIXEL_COLOR_SAMPLE, Lay_MoveTableY, MoveY },
		{ IDC_PIXEL_INFO_STATIC, SizeX | Lay_MoveTableY, SizeX | MoveY },
		{ IDC_SHOW_COLOR_BOARD_CHECK, MoveX | Lay_MoveTableY, MoveX | MoveY },

		{ IDC_COLOR_BOARD_LABEL, Lay_MoveTableY, MoveY },
		{ IDC_COLOR_BOARD_MODE_COMBO, Lay_MoveTableY | CollapsedTop, MoveY },
		{ IDC_UNIQUE_COLORS_CHECK, Lay_MoveTableY, MoveY },
		{ IDC_SHOW_COLOR_LABELS_CHECK, Lay_MoveTableY, MoveY },
		{ IDC_COLOR_BOARD_INFO_STATIC, SizeX | Lay_MoveTableY, SizeX | MoveY },
		{ IDC_COLOR_BOARD_SAMPLE, SizeX | Lay_MoveTableY | Lay_SizeTableY, SizeX | MoveY },

		{ IDCANCEL, MoveX, UseExpanded },
		{ ID_REFRESH, MoveX, UseExpanded }
	};
}

CImageDialog::CImageDialog( CWnd* pParent )
	: CLayoutDialog( IDD_IMAGE_DIALOG, pParent )
	, m_sampleMode( (SampleMode)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_sampleMode, ShowImage ) )
	, m_imagingApi( (ui::ImagingApi)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_imagingApi, ui::WicApi ) )
	, m_framePos( AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_framePos, 1 ) )
	, m_frameCount( 1 )
	, m_forceResolution( (Resolution)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_forceResolution, Auto ) )
	, m_showFlags( AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_showFlags, ShowGuides | ShowLabels ) )
	, m_zoom( (Zoom)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_zoom, Zoom100 ) )
	, m_statusAlpha( (BYTE)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_statusAlpha, 127 ) )
	, m_multiZone( (CMultiZone::Stacking)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_stacking, CMultiZone::Auto ) )
	, m_imagePath( AfxGetApp()->GetProfileString( reg::section_dlg, reg::entry_imagePath ) )

	, m_imagePathCombo( ui::FilePath, gdi::g_imageFileFilter )
	, m_pBkColorPicker( new CColorPickerButton() )
	, m_imagingApiCombo( &ui::GetTags_ImagingApi() )
	, m_forceResolutionCombo( &GetTags_Resolution() )
	, m_zoomCombo( &GetTags_Zoom() )
	, m_stackingCombo( &CMultiZone::GetTags_Stacking() )
	, m_colorBoardModeCombo( &CColorBoard::GetTags_Mode() )

	, m_transpColorSample( this )
	, m_pImageToolbar( new CDialogToolBar() )
	, m_sampleView( this )
	, m_pPixelInfoSample( new CPixelInfoSample() )
	, m_pColorBoardSample( new CColorBoardSample( &m_colorBoard, this ) )
	, m_statusAlphaEdit( IDC_STATUS_ALPHA_EDIT, &m_statusAlpha )
	, m_modeSheet( m_sampleMode )

	, m_convertFlags( (BYTE)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_convertFlags, 127 ) )
	, m_sourceAlpha( (BYTE)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_sourceAlpha, 127 ) )
	, m_pixelAlpha( (BYTE)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_pixelAlpha, 127 ) )
	, m_disabledAlpha( (BYTE)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_disabledAlpha, 127 ) )
	, m_contrastPct( (TPercent)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_contrastPct, 30 ) )
{
	m_regSection = reg::section_dlg;
	GetLayoutEngine().RegisterDualCtrlLayout( ARRAY_SPAN( layout::s_dualStyles ) );
	LoadDlgIcon( ID_STUDY_IMAGE );
	m_accelPool.AddAccelTable( new CAccelTable( IDD_IMAGE_DIALOG ) );
	m_initCentered = false;			// so that it uses WINDOWPLACEMENT
	ui::LoadPopupMenu( &m_contextMenu, IDR_CONTEXT_MENU, app::ImageDialogPopup );

	m_multiZone.m_zoneSpacing = AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_spacing, m_multiZone.m_zoneSpacing );
	m_colorBoard.Load( reg::section_dlg );
	m_transpColorCache.Load( reg::section_dlg );

	m_imagePathCombo.SetEnsurePathExist();
	m_pBkColorPicker->SetColor( (COLORREF)AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_bkColor, CLR_NONE ) );
	m_pBkColorPicker->SetAutoColor( ui::MakeSysColor( COLOR_BTNFACE ) );
	m_spacingEdit.SetValidRange( Range<int>( 0, 500 ) );
	m_pImageToolbar->GetStrip()
		.AddButton( ID_SET_AUTO_TRANSP_COLOR, ID_AUTO_TRANSP_TOOL )
		.AddButton( ID_RESET_TRANSP_COLOR, ID_REMOVE_ITEM );

	//m_sampleView.SetBorderColor( color::LightBlue );		// for debugging

	m_modeData.resize( _ModeCount );
	m_modeData[ ShowImage ] = new CModeData( _T("Original") );
	m_modeData[ ConvertImage ] = new CModeData( _T("Original|To 1 bit (monochrome)|To 4 bit (16 colors)|To 8 bit (256 colors)|To 16 bit (64K colors)|To 24 bit (True Color)|To 32 bit (True Color w. Alpha)") );
	m_modeData[ ContrastImage ] = new CModeData( _T("Original|Contrast Adjusted") );
	m_modeData[ GrayScale ] = new CModeData( _T("Original|Gray scale") );
	m_modeData[ AlphaBlend ] = new CModeData( _T("Original|Alpha-blend with source alpha|Pixel alpha-blended|Pixel gradient") );
	m_modeData[ BlendColor ] = new CModeData( _T("Original|Blended with background") );
	m_modeData[ Disabled ] = new CModeData( _T("Original|Gray scale|Blended with background|Disabled Faded|Disabled Gray|Disable Fade Gray (smooth)") );
	m_modeData[ ImageList ] = new CModeData( _T("Original|Disabled|Embossed|Blended 25%|Blended 50%|Blended 75%") );
	m_modeData[ RectsAlphaBlend ] = new CModeData( _T("") );

	m_modeSheet.AddPage( new CShowImagePage( this ) );
	m_modeSheet.AddPage( new CConvertModePage( this ) );
	m_modeSheet.AddPage( new CContrastModePage( this ) );
	m_modeSheet.AddPage( CModePage::NewEmptyPage( this, GrayScale ) );
	m_modeSheet.AddPage( new CAlphaBlendModePage( this ) );
	m_modeSheet.AddPage( new CBlendColorModePage( this ) );
	m_modeSheet.AddPage( new CDisabledModePage( this ) );
	m_modeSheet.AddPage( CModePage::NewEmptyPage( this, ImageList ) );
	m_modeSheet.AddPage( CModePage::NewEmptyPage( this, RectsAlphaBlend ) );

#ifdef _DEBUG
	// test mode flags
//	SetFlag( CDibSection::s_testFlags, CDibSection::ForceCvtEqualBpp );
#endif
}

CImageDialog::~CImageDialog()
{
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_sampleMode, m_sampleMode );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_imagingApi, m_imagingApi );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_showFlags, m_showFlags );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_zoom, m_zoom );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_framePos, m_framePos );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_forceResolution, m_forceResolution );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_statusAlpha, m_statusAlpha );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_stacking, m_multiZone.GetStacking() );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_spacing, m_multiZone.m_zoneSpacing );
	AfxGetApp()->WriteProfileString( reg::section_dlg, reg::entry_imagePath, m_imagePath.c_str() );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_convertFlags, m_convertFlags );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_sourceAlpha, m_sourceAlpha );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_pixelAlpha, m_pixelAlpha );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_disabledAlpha, m_disabledAlpha );
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_contrastPct, m_contrastPct );

	m_colorBoard.Save( reg::section_dlg );
	m_transpColorCache.Save( reg::section_dlg );

	utl::ClearOwningContainer( m_modeData );
}

const CEnumTags& CImageDialog::GetTags_SampleMode( void )
{
	static const CEnumTags s_tags( _T("Show Image|Convert Image|Contrast|Gray Scale|Alpha Blend|Blend Color|Disabled|Image List|Blend Rects") );
	return s_tags;
}

const CEnumTags& CImageDialog::GetTags_Resolution( void )
{
	static const CEnumTags s_tags( _T("Do Not Change|1 bit (monochrome)|4 bit (16 colors)|8 bit (256 colors)|16 bit (64K colors)|24 bit (True Color)|32 bit (True Color w. Alpha)") );
	return s_tags;
}

const CEnumTags& CImageDialog::GetTags_Zoom( void )
{
	static const CEnumTags s_tags( _T("25%|50%|100%|200%|400%|800%|1600%|3200%|Stretch to Fit") );
	return s_tags;
}

WORD CImageDialog::GetForceBpp( void ) const
{
	static const WORD s_bpp[] = { 0, 1, 4, 8, 16, 24, 32 };		// indexed by Resolution

	ASSERT( m_forceResolution < COUNT_OF( s_bpp ) );
	return s_bpp[ m_forceResolution ];
}

COLORREF CImageDialog::GetBkColor( void ) const
{
	return m_pBkColorPicker->GetDisplayColor();
}

int CImageDialog::GetZoomPct( void ) const
{
	switch ( m_zoom )
	{
		default: ASSERT( false );
		case StretchToFit:
		case Zoom100:
			return 100;
		case Zoom25:	return 25;
		case Zoom50:	return 50;
		case Zoom200:	return 200;
		case Zoom400:	return 400;
		case Zoom800:	return 800;
		case Zoom1600:	return 1600;
		case Zoom3200:	return 3200;
	}
}

void CImageDialog::LoadSampleImage( void )
{
	CWaitCursor wait;
	if ( ui::WicApi == m_imagingApi )
		m_frameCount = wic::QueryImageFrameCount( m_imagePath.c_str() );
	else
		m_framePos = m_frameCount = 1;

	m_frameCount = std::max( 1u, m_frameCount );
	m_framePos = std::max( 1u, m_framePos );
	m_framePos = std::min( m_frameCount, m_framePos );

	m_pDibSection.reset( new CDibSection() );
	if ( m_pDibSection->LoadFromFile( m_imagePath.c_str(), m_imagingApi, m_framePos - 1 ) )
	{
		m_pDibSection->SetTranspColor( m_transpColorCache.Lookup( m_imagePath ) );		// set the cached transparent color

		if ( m_forceResolution != Auto )
			if ( WORD forceBpp = GetForceBpp() )
				if ( forceBpp != m_pDibSection->GetBitsPerPixel() ||
					 HasFlag( m_convertFlags, CDibSection::ForceCvtEqualBpp ) )
				{
					CScopedFlag<int> scopedSkipCopyImage( &CDibSection::s_testFlags, m_convertFlags & CDibSection::ForceCvtEqualBpp );
					std::auto_ptr<CDibSection> pCvtDib( new CDibSection() );
					pCvtDib->Convert( *m_pDibSection, forceBpp );
					m_pDibSection.reset( pCvtDib.release() );
				}
	}
	else
		m_pDibSection.reset();

	m_framePosEdit.SetValidRange( Range<int>( 1, m_frameCount ) );
	m_framePosEdit.SetNumber<UINT>( m_framePos );
	UpdateImage();
}

void CImageDialog::UpdateImage( void )
{
	if ( m_pDibSection.get() != nullptr )
	{
		m_transpColorCache.Register( m_imagePath, m_pDibSection->GetTranspColor() );

		ui::SetDlgItemText( m_hWnd, IDC_FRAME_COUNT_STATIC, str::Format( _T("of %d"), m_frameCount ) );
		ui::SetDlgItemText( m_hWnd, IDC_IMAGE_INFO_STATIC, FormatDibInfo( *m_pDibSection ) );

		ui::ShowWindow( m_transpColorSample, true );
		ui::EnableWindow( m_transpColorSample, m_pDibSection->HasTranspColor() );
		m_transpColorSample.SetColor( m_pDibSection->GetTranspColor() );
	}
	else
	{
		ui::SetDlgItemText( m_hWnd, IDC_FRAME_COUNT_STATIC, std::tstring() );
		ui::SetDlgItemText( m_hWnd, IDC_IMAGE_INFO_STATIC, std::tstring() );
		ui::ShowWindow( m_transpColorSample, false );
	}
	m_pImageToolbar->UpdateCmdUI();

	BuildColorBoard();
	CreateEffectDibs();

	m_modeSheet.OutputPage( ShowImage );			// update transparent color info
	RedrawSample();
}

std::tstring CImageDialog::FormatDibInfo( const CDibSection& dib )
{
	CBitmapInfo info( dib.GetBitmapHandle() );
	std::tstring text = str::Format( _T("%d x %d | Source: %d bit%s | DIB: %d bit - %s KB (%s bytes)"),
		info.dsBmih.biWidth,
		info.dsBmih.biHeight,
		dib.GetSrcMeta().m_bitsPerPixel,
		dib.HasAlpha() ? _T(", Alpha Channel") : _T(""),
		info.dsBmih.biBitCount,
		num::FormatNumber( info.dsBmih.biSizeImage >> 10, str::GetUserLocale() ).c_str(),		// KB
		num::FormatNumber( info.dsBmih.biSizeImage, str::GetUserLocale() ).c_str() );			// bytes

	if ( dib.HasTranspColor() )
		text += str::Format( _T(" | %s color: %s"),
			dib.HasAutoTranspColor() ? _T("Auto-transparent") : _T("Transparent"),
			utl::FormatHexColor( dib.GetTranspColor() ).c_str() );
	return text;
}

void CImageDialog::RedrawSample( void )
{
	static const UINT frameCtrls[] = { IDC_FRAME_POS_LABEL, IDC_FRAME_POS_EDIT, IDC_FRAME_COUNT_STATIC };
	ui::EnableControls( m_hWnd, frameCtrls, COUNT_OF( frameCtrls ), ui::WicApi == m_imagingApi && IsValidImageMode() && m_frameCount > 1 );
	ui::EnableControl( m_hWnd, IDC_ZOOM_COMBO, IsValidImageMode() );

	m_sampleView.SetContentSize( ComputeContentSize() );
	m_sampleView.SafeRedraw();
}

void CImageDialog::BuildColorBoard( void )
{
	m_colorBoard.Build( m_pDibSection.get() );

	std::tstring text;
	if ( !m_colorBoard.IsEmpty() )
		if ( m_colorBoard.m_totalColors == m_colorBoard.m_colors.size() )
		{
			text = str::Format( _T("%d colors"), m_colorBoard.m_colors.size() );
			if ( !m_colorBoard.m_dupColorsPos.empty() )
				text += str::Format( _T(" (%d unique + %d duplicates)"), m_colorBoard.m_colors.size() - m_colorBoard.m_dupColorsPos.size(), m_colorBoard.m_dupColorsPos.size() );
		}
		else
			text = str::Format( _T("%d colors (of total %d)"), m_colorBoard.m_colors.size(), m_colorBoard.m_totalColors );

	ui::SetDlgItemText( m_hWnd, IDC_COLOR_BOARD_INFO_STATIC, text );

	RedrawColorBoard();
}

void CImageDialog::RedrawColorBoard( void )
{
	m_pColorBoardSample->SafeRedraw();
}

bool CImageDialog::IsValidImageMode( void ) const
{
	return m_pDibSection.get() != nullptr && m_sampleMode != RectsAlphaBlend;
}

CSize CImageDialog::ComputeContentSize( void )
{
	m_multiZone.Clear();

	const CModeData* pModeData = m_modeData[ m_sampleMode ];
	if ( RectsAlphaBlend == m_sampleMode )
	{
		m_multiZone.Init( CSize( 10, 10 ), pModeData->GetZoneCount() );
		m_multiZone.SetStacking( CMultiZone::VertStacked );
	}
	else if ( m_pDibSection.get() != nullptr )
	{
		m_multiZone.Init( ui::ScaleSize( m_pDibSection->GetSize(), GetZoomPct(), 100 ) + GetModeExtraSpacing(), pModeData->GetZoneCount() );
		m_multiZone.SetStacking( m_stackingCombo.GetEnum<CMultiZone::Stacking>() );

		if ( !StretchImage() )
			return m_multiZone.GetTotalSize();
	}

	return CSize( 0, 0 );			// not scrollable
}

bool CImageDialog::RenderBackground( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) implements(ui::ISampleCallback)
{
	pCtrl;
	CBrush bkBrush( GetBkColor() );

	pDC->FillRect( &boundsRect, &bkBrush );
	return true;
}

bool CImageDialog::RenderSample( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) implements(ui::ISampleCallback)
{
	pCtrl;
	if ( RectsAlphaBlend == m_sampleMode )
		return Render_RectsAlphaBlend( pDC, boundsRect );

	if ( nullptr == m_pDibSection.get() && m_sampleMode != RectsAlphaBlend )
		return false;

	CRect contentRect = MakeContentRect( boundsRect );
	CMultiZoneIterator itZone( m_multiZone, contentRect );

	switch ( m_sampleMode )
	{
		case ShowImage:
		case ConvertImage:
		case ContrastImage:
		case GrayScale:
		case BlendColor:
		case Disabled:
			Render_AllImages( pDC, &itZone );
			break;
		case AlphaBlend:
			Render_AlphaBlend( pDC, &itZone );
			break;
		case ImageList:
			Render_ImageList( pDC, &itZone );
			break;
		default: return false;
	}

	const CModeData* pModeData = m_modeData[ m_sampleMode ];

	if ( HasFlag( m_showFlags, ShowLabels ) && pModeData->GetZoneCount() > 1 )
	{
		CScopedDrawText scopedDrawText( pDC, &m_sampleView, GetFont(), color::White, color::Gray60 );
		itZone.DrawLabels( pDC, boundsRect, pModeData->m_labels );
	}

	if ( HasFlag( m_showFlags, ShowGuides ) && m_sampleMode != ImageList )
	{
		itZone.Restart();
		for ( unsigned int i = 0; i != itZone.GetCount(); ++i )
			m_sampleView.DrawContentFrame( pDC, itZone.GetNextZone(), color::Red, 64 );

		//m_sampleView.DrawContentFrame( pDC, contentRect, color::Red );
	}

	return true;
}

CSize CImageDialog::GetModeExtraSpacing( void ) const
{
	CSize modeSpacing( 0, 0 );
	if ( ImageList == m_sampleMode )
		modeSpacing.cx = IL_SpacingX * ( m_pImageList->GetImageCount() - 1 );
	return modeSpacing;
}

CRect CImageDialog::MakeContentRect( const CRect& clientRect ) const
{
	return StretchImage()
		? ui::StretchToFit( clientRect, m_multiZone.GetTotalSize(), m_multiZone.GetSpacingSize() + GetModeExtraSpacing() )		// stretch image area but keep spacing as is
		: m_sampleView.MakeDisplayRect( clientRect, m_multiZone.GetTotalSize() );
}

void CImageDialog::ShowPixelInfo( const CPoint& pos, COLORREF color, CWnd* pCtrl ) implements(ui::ISampleCallback)
{
	pCtrl;
	std::tstring text;
	if ( m_pDibSection.get() != nullptr || RectsAlphaBlend == m_sampleMode )
	{
		m_pPixelInfoSample->SetPixelInfo( color, pos, this );

		if ( color != CLR_NONE )
		{
			size_t oldSelPos = m_colorBoard.m_selPos;
			m_colorBoard.SelectColor( color );
			text = utl::FormatColorInfo( color, m_colorBoard.m_selPos, pos );

			if ( m_colorBoard.m_selPos != oldSelPos )
				RedrawColorBoard();
		}
	}
	else
		m_pPixelInfoSample->Reset();

	ui::SetDlgItemText( m_hWnd, IDC_PIXEL_INFO_STATIC, text );
}

void CImageDialog::DoDataExchange( CDataExchange* pDX ) override
{
	bool firstInit = nullptr == m_imagePathCombo.m_hWnd;

	m_transpColorSample.DDX_Placeholder( pDX, IDC_TRANSP_COLOR_SAMPLE );
	m_sampleView.DDX_Placeholder( pDX, IDC_IMAGE_SAMPLE );
	m_pPixelInfoSample->DDX_Placeholder( pDX, IDC_PIXEL_COLOR_SAMPLE );
	m_pColorBoardSample->DDX_Placeholder( pDX, IDC_COLOR_BOARD_SAMPLE );
	m_pImageToolbar->DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignRight | V_AlignCenter );

	DDX_Control( pDX, IDC_IMAGE_PATH_COMBO, m_imagePathCombo );
	DDX_Control( pDX, IDC_FRAME_POS_EDIT, m_framePosEdit );
	ui::DDX_ColorEditor( pDX, IDC_BK_COLOR_PICKER_BUTTON, m_pBkColorPicker.get(), nullptr );
	ui::DDX_Flag( pDX, IDC_FORCE_CVT_EQUAL_BPP_CHECK, m_convertFlags, CDibSection::ForceCvtEqualBpp );
	ui::DDX_Flag( pDX, IDC_SHOW_GUIDES_CHECK, m_showFlags, ShowGuides );
	ui::DDX_Flag( pDX, IDC_SHOW_LABELS_CHECK, m_showFlags, ShowLabels );
	m_spacingEdit.DDX_Number( pDX, m_multiZone.m_zoneSpacing, IDC_SPACING_EDIT );
	m_statusAlphaEdit.DDX_Channel( pDX );
	ui::DDX_ButtonIcon( pDX, ID_REFRESH );
	m_modeSheet.DDX_DetailSheet( pDX, IDC_SAMPLE_MODE_SHEET, true );

	if ( firstInit )
	{
		DragAcceptFiles();
		m_imagePathCombo.LoadHistory( reg::section_dlg, reg::entry_imagePathHistory );

		m_framePosEdit.SetWrap();

		CSpinButtonCtrl* pSpin = m_framePosEdit.GetSpinButtonCtrl();
		UDACCEL accel[] = { { 0, 1 }, { 15, 5 } };
		pSpin->SetAccel( COUNT_OF( accel ), accel );
	}

	ui::DDX_Text( pDX, IDC_IMAGE_PATH_COMBO, m_imagePath );
	m_imagingApiCombo.DDX_EnumValue( pDX, IDC_IMAGE_API_COMBO, m_imagingApi );
	m_forceResolutionCombo.DDX_EnumValue( pDX, IDC_FORCE_RESOLUTION_COMBO, m_forceResolution );
	m_zoomCombo.DDX_EnumValue( pDX, IDC_ZOOM_COMBO, m_zoom );
	m_stackingCombo.DDX_EnumValue( pDX, IDC_STACKING_COMBO, m_multiZone.RefStacking() );
	m_colorBoardModeCombo.DDX_EnumValue( pDX, IDC_COLOR_BOARD_MODE_COMBO, m_colorBoard.m_mode );
	ui::DDX_Bool( pDX, IDC_UNIQUE_COLORS_CHECK, m_colorBoard.m_uniqueColors );
	ui::DDX_Bool( pDX, IDC_SHOW_COLOR_LABELS_CHECK, m_colorBoard.m_showLabels );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		LoadSampleImage();
	else
	{
		m_modeSheet.SetSheetModified();
		m_modeSheet.ApplyChanges();
		m_sampleMode = static_cast<SampleMode>( m_modeSheet.GetActiveIndex() );
	}

	__super::DoDataExchange( pDX );

	// post dialog init
	if ( firstInit )
		CheckDlgButton( IDC_SHOW_COLOR_BOARD_CHECK, !GetLayoutEngine().IsCollapsed() );
}


// message handlers

BEGIN_MESSAGE_MAP( CImageDialog, CLayoutDialog )
	ON_WM_DROPFILES()
	ON_WM_CONTEXTMENU()
	ON_WM_CTLCOLOR()
	ON_CN_DETAILSCHANGED( IDC_IMAGE_PATH_COMBO, OnChange_ImageFile )
	ON_CBN_EDITCHANGE( IDC_IMAGE_PATH_COMBO, OnChange_ImageFile )
	ON_CBN_SELCHANGE( IDC_IMAGE_PATH_COMBO, OnChange_ImageFile )
	ON_EN_CHANGE( IDC_FRAME_POS_EDIT, OnChange_FramePos )
	ON_BN_CLICKED( IDC_BK_COLOR_PICKER_BUTTON, OnChange_BkColor )
	ON_CBN_SELCHANGE( IDC_IMAGE_API_COMBO, OnChange_ImagingApi )
	ON_CBN_SELCHANGE( IDC_FORCE_RESOLUTION_COMBO, OnChange_ForceResolution )
	ON_BN_CLICKED( ID_RESOLUTION_TOGGLE, OnToggle_ForceResolution )
	ON_BN_CLICKED( IDC_FORCE_CVT_EQUAL_BPP_CHECK, OnToggle_SkipCopyImage )
	ON_CBN_SELCHANGE( IDC_ZOOM_COMBO, OnChange_Zoom )
	ON_BN_CLICKED( ID_ZOOM_TOGGLE, OnToggle_Zoom )
	ON_CBN_SELCHANGE( IDC_STACKING_COMBO, OnChange_Stacking )
	ON_CONTROL_RANGE( BN_CLICKED, IDC_SHOW_GUIDES_CHECK, IDC_SHOW_LABELS_CHECK, OnToggle_ShowFlags )
	ON_BN_CLICKED( IDC_SHOW_COLOR_BOARD_CHECK, OnToggle_ShowColorBoard )
	ON_EN_CHANGE( IDC_SPACING_EDIT, OnChange_Spacing )
	ON_EN_CHANGE( IDC_STATUS_ALPHA_EDIT, OnRedrawSample )
	ON_CBN_SELCHANGE( IDC_COLOR_BOARD_MODE_COMBO, OnChange_ColorBoardMode )
	ON_BN_CLICKED( IDC_UNIQUE_COLORS_CHECK, OnToggle_UniqueColors )
	ON_BN_CLICKED( IDC_SHOW_COLOR_LABELS_CHECK, OnToggle_ShowLabels )
	ON_BN_CLICKED( ID_SET_TRANSP_COLOR, OnSetTranspColor )
	ON_BN_CLICKED( ID_SET_AUTO_TRANSP_COLOR, OnSetAutoTranspColor )
	ON_BN_CLICKED( ID_RESET_TRANSP_COLOR, OnResetTranspColor )
	ON_UPDATE_COMMAND_UI_RANGE( ID_SET_TRANSP_COLOR, ID_RESET_TRANSP_COLOR, OnUpdateTranspColor )
	ON_BN_CLICKED( ID_REFRESH, OnRedrawSample )
END_MESSAGE_MAP()

void CImageDialog::OnOK( void ) override
{
	// Notifications are disabled for the focused control during UpdateData() by MFC.
	// Save history combos just before UpdateData( DialogSaveChanges ), so that HCN_VALIDATEITEMS notifications are received.
	m_imagePathCombo.SaveHistory( reg::section_dlg, reg::entry_imagePathHistory );

	__super::OnOK();
}

void CImageDialog::OnDestroy( void )  override
{
	AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_bkColor, m_pBkColorPicker->GetColor() );

	__super::OnDestroy();
}

void CImageDialog::OnDropFiles( HDROP hDropInfo )
{
	TCHAR filePath[ MAX_PATH ];
	::DragQueryFile( hDropInfo, 0, filePath, MAX_PATH );
	::DragFinish( hDropInfo );

	if ( fs::IsValidFile( filePath ) )
		if ( wic::QueryImageFrameCount( filePath ) != 0 )		// valid image file
		{
			ui::SetComboEditText( m_imagePathCombo, filePath );
			OnChange_ImageFile();
			SetForegroundWindow();
		}
		else
			ui::BeepSignal( MB_ICONWARNING );
}

void CImageDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
	pWnd;
	m_contextMenu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
}

HBRUSH CImageDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType )
{
	HBRUSH hBrushFill = __super::OnCtlColor( pDC, pWnd, ctlColorType );
	if ( ctlColorType == CTLCOLOR_STATIC && pWnd != nullptr )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_IMAGE_INFO_STATIC:
			case IDC_COLOR_BOARD_INFO_STATIC:
				pDC->SetTextColor( color::Blue );
				break;
			case IDC_FORCE_RESOLUTION_LABEL:
				if ( m_forceResolution != Auto )
					pDC->SetTextColor( color::Red );
				break;
		}

	return hBrushFill;
}

void CImageDialog::OnChange_ImageFile( void )
{
	m_imagePathCombo.UpdateWindow();		// instant visual feedback for long loads
	std::tstring imagePath = ui::GetComboSelText( m_imagePathCombo );
	if ( imagePath != m_imagePath )
	{
		m_imagePath = imagePath;
		LoadSampleImage();
	}
}

void CImageDialog::OnChange_FramePos( void )
{
	if ( m_framePosEdit.GetNumber<UINT>() != m_framePos )
	{
		m_framePos = m_framePosEdit.GetNumber<UINT>();
		m_framePosEdit.UpdateWindow();		// instant feedback
		LoadSampleImage();
	}
}

void CImageDialog::OnChange_BkColor( void )
{
	RedrawSample();
}

void CImageDialog::OnChange_SampleMode( void )
{
	m_sampleMode = static_cast<SampleMode>( m_modeSheet.GetActiveIndex() );
	CreateEffectDibs();
	RedrawSample();
}

void CImageDialog::OnChange_ImagingApi( void )
{
	m_imagingApi = m_imagingApiCombo.GetEnum<ui::ImagingApi>();
	m_transpColorCache.Unregister( m_imagePath );			// reset transparent color
	LoadSampleImage();
}

void CImageDialog::OnChange_ForceResolution( void )
{
	m_forceResolution = m_forceResolutionCombo.GetEnum<Resolution>();
	LoadSampleImage();
	GetDlgItem( IDC_FORCE_RESOLUTION_LABEL )->Invalidate();
}

void CImageDialog::OnToggle_ForceResolution( void )
{
	m_forceResolutionCombo.SetValue( Auto == m_forceResolution ? Color24 : Auto );
	OnChange_ForceResolution();
}

void CImageDialog::OnToggle_SkipCopyImage( void )
{
	SetFlag( m_convertFlags, CDibSection::ForceCvtEqualBpp, IsDlgButtonChecked( IDC_FORCE_CVT_EQUAL_BPP_CHECK ) != FALSE );
	LoadSampleImage();
}

void CImageDialog::OnChange_Zoom( void )
{
	m_zoom = m_zoomCombo.GetEnum<Zoom>();
	RedrawSample();
}

void CImageDialog::OnToggle_Zoom( void )
{
	m_zoomCombo.SetValue( m_zoom != StretchToFit ? StretchToFit : Zoom100 );
	OnChange_Zoom();
}

void CImageDialog::OnChange_Stacking( void )
{
	m_multiZone.SetStacking( m_stackingCombo.GetEnum<CMultiZone::Stacking>() );
	RedrawSample();
}

void CImageDialog::OnToggle_ShowFlags( UINT checkId )
{
	switch ( checkId )
	{
		case IDC_SHOW_GUIDES_CHECK: SetFlag( m_showFlags, ShowGuides, IsDlgButtonChecked( checkId ) != FALSE ); break;
		case IDC_SHOW_LABELS_CHECK: SetFlag( m_showFlags, ShowLabels, IsDlgButtonChecked( checkId ) != FALSE ); break;
		default: ASSERT( false );
	}
	m_sampleView.Invalidate();
}

void CImageDialog::OnToggle_ShowColorBoard( void )
{
	GetLayoutEngine().SetCollapsed( !IsDlgButtonChecked( IDC_SHOW_COLOR_BOARD_CHECK ) );
}

void CImageDialog::OnChange_Spacing( void )
{
	m_multiZone.m_zoneSpacing = m_spacingEdit.GetNumericValue();
	RedrawSample();
}

void CImageDialog::OnChange_ColorBoardMode( void )
{
	m_colorBoard.m_mode = m_colorBoardModeCombo.GetEnum<CColorBoard::Mode>();
	BuildColorBoard();
}

void CImageDialog::OnToggle_UniqueColors( void )
{
	m_colorBoard.m_uniqueColors = IsDlgButtonChecked( IDC_UNIQUE_COLORS_CHECK ) != FALSE;
	BuildColorBoard();
}

void CImageDialog::OnToggle_ShowLabels( void )
{
	m_colorBoard.m_showLabels = IsDlgButtonChecked( IDC_SHOW_COLOR_LABELS_CHECK ) != FALSE;
	RedrawColorBoard();
}

void CImageDialog::OnSetTranspColor( void )
{
	m_pDibSection->SetTranspColor( m_pPixelInfoSample->GetColor() );
	UpdateImage();
}

void CImageDialog::OnSetAutoTranspColor( void )
{
	m_pDibSection->SetAutoTranspColor();
	UpdateImage();
}

void CImageDialog::OnResetTranspColor( void )
{
	m_pDibSection->SetTranspColor( CLR_NONE );
	UpdateImage();
}

void CImageDialog::OnUpdateTranspColor( CCmdUI* pCmdUI )
{
	enum CurrTransp { TranspColor, AutoTranspColor, NoTranspColor } transp = NoTranspColor;
	if ( m_pDibSection.get() != nullptr && m_pDibSection->HasTranspColor() )
		transp = m_pDibSection->HasAutoTranspColor() ? AutoTranspColor : TranspColor;
	CurrTransp btnTransp = (CurrTransp)( pCmdUI->m_nID - ID_SET_TRANSP_COLOR );

	ui::SetRadio( pCmdUI, transp == btnTransp );
	pCmdUI->Enable( m_pDibSection.get() != nullptr && !m_pDibSection->HasAlpha() );
}

void CImageDialog::OnRedrawSample( void )
{
	RedrawSample();
}


// CColorBoardSample implementation

bool CColorBoardSample::RenderSample( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl )
{
	pCtrl;
	CScopedDrawText scopedDrawText( pDC, this, GetParent()->GetFont() );
	m_pColorBoard->Draw( pDC, boundsRect );
	return true;
}

// CModePage implementation

CModePage::CModePage( CImageDialog* pDialog, CImageDialog::SampleMode mode, UINT templateId )
	: CLayoutPropertyPage( templateId )
	, m_pDialog( pDialog )
{
	SetUseLazyUpdateData( false );		// no update on page activation
	SetTitle( CImageDialog::GetTags_SampleMode().GetUiTags()[ mode ] );
}

CModePage::~CModePage()
{
	utl::ClearOwningContainer( m_channelEdits );
}

CModePage* CModePage::NewEmptyPage( CImageDialog* pDialog, CImageDialog::SampleMode mode )
{
	return new CModePage( pDialog, mode, IDD_IMAGE_PAGE_EMPTY );
}

bool CModePage::IsPixelCtrl( UINT ctrlId ) const
{
	if ( const CColorChannelEdit* pChannelEdit = dynamic_cast<const CColorChannelEdit*>( GetDlgItem( ctrlId ) ) )
		return pChannelEdit->IsPixelChannel();
	return false;
}

bool CModePage::SyncEditValues( const CColorChannelEdit* pRefEdit )
{
	if ( nullptr == pRefEdit )
		pRefEdit = m_keepEqualEdits.front();
	else if ( !utl::Contains( m_keepEqualEdits, pRefEdit ) )
		return false;

	ASSERT_PTR( pRefEdit );

	for ( std::vector<CColorChannelEdit*>::const_iterator itEdit = m_keepEqualEdits.begin(); itEdit != m_keepEqualEdits.end(); ++itEdit )
		if ( *itEdit != pRefEdit )
			( *itEdit )->SyncValueWith( pRefEdit );

	return true;
}

void CModePage::DoDataExchange( CDataExchange* pDX )
{
	for ( std::vector<CColorChannelEdit*>::const_iterator itChannelEdit = m_channelEdits.begin(); itChannelEdit != m_channelEdits.end(); ++itChannelEdit )
		( *itChannelEdit )->DDX_Channel( pDX );

	__super::DoDataExchange( pDX );
}

BOOL CModePage::OnSetActive( void )
{
	m_pDialog->OnChange_SampleMode();
	return __super::OnSetActive();
}

BEGIN_MESSAGE_MAP( CModePage, CLayoutPropertyPage )
	ON_CONTROL_RANGE( EN_CHANGE, IDC_CHANNEL1_EDIT, IDC_CHANNEL3_EDIT, OnChangeField )
	ON_BN_CLICKED( IDC_KEEP_EQUAL_CHECK, OnToggle_KeepEqual )
END_MESSAGE_MAP()

void CModePage::OnChangeField( UINT ctrlId )
{
	SyncEditValues( checked_static_cast<const CColorChannelEdit*>( GetDlgItem( ctrlId ) ) );

	// input is done by the edit
	if ( IsPixelCtrl( ctrlId ) )
		m_pDialog->OnChange_SampleMode();
	else
		m_pDialog->OnRedrawSample();
}

void CModePage::OnToggle_KeepEqual( void )
{
	if ( !m_keepEqualEdits.empty() && IsDlgButtonChecked( IDC_KEEP_EQUAL_CHECK ) )
		SyncEditValues( dynamic_cast<CColorChannelEdit*>( GetFocus() ) );
}

void CModePage::OnPageInput( UINT checkId )
{
	checkId;
	UpdateData( DialogSaveChanges );
}


// CShowImagePage implementation

CShowImagePage::CShowImagePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::ShowImage, IDD_IMAGE_PAGE_SHOW_IMAGE )
	, m_transpColorSample( pDialog )
{
	static const CLayoutStyle styles[] = { { IDC_TRANSP_COLOR_INFO, layout::SizeX } };
	RegisterCtrlLayout( styles, COUNT_OF( styles ) );
}

void CShowImagePage::DoDataExchange( CDataExchange* pDX )
{
	m_transpColorSample.DDX_Placeholder( pDX, IDC_TRANSP_COLOR_SAMPLE );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		std::tstring info;
		if ( m_pDialog->GetImage() != nullptr )
		{
			COLORREF transpColor = m_pDialog->GetImage()->GetTranspColor();
			m_transpColorSample.SetColor( transpColor );
			if ( transpColor != CLR_NONE )
				info = utl::FormatColorInfo( transpColor, m_pDialog->GetColorBoard().FindColorPos( transpColor ) );
		}
		ui::SetDlgItemText( this, IDC_TRANSP_COLOR_INFO, info );
	}

	__super::DoDataExchange( pDX );
}


// CConvertModePage implementation

CConvertModePage::CConvertModePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::ConvertImage, IDD_IMAGE_PAGE_CONVERT_IMAGE )
{
}

void CConvertModePage::DoDataExchange( CDataExchange* pDX )
{
//	ui::DDX_Flag( pDX, IDC_FORCE_CVT_EQUAL_BPP_CHECK, m_pDialog->m_convertFlags, CDibSection::ForceCvtEqualBpp );
	ui::DDX_Flag( pDX, IDC_FORCE_CVT_COPY_PIXELS_CHECK, m_pDialog->m_convertFlags, CDibSection::ForceCvtCopyPixels );
	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		m_pDialog->ConvertImages();

	__super::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CConvertModePage, CModePage )
	ON_CONTROL_RANGE( BN_CLICKED, IDC_FORCE_CVT_EQUAL_BPP_CHECK, IDC_FORCE_CVT_COPY_PIXELS_CHECK, OnPageInput )
END_MESSAGE_MAP()


// CContrastModePage implementation

CContrastModePage::CContrastModePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::ContrastImage, IDD_IMAGE_PAGE_CONTRAST )
	, m_contrastPctEdit( IDC_CONTRAST_PCT_EDIT, &m_pDialog->m_contrastPct )
{
}

void CContrastModePage::DoDataExchange( CDataExchange* pDX )
{
	m_contrastPctEdit.DDX_Percent( pDX );

	__super::DoDataExchange( pDX );
}

void CContrastModePage::OnChange_ContrastPct( void )
{
	// input is done by the edit
	m_pDialog->ConvertImages();		// recreated the DIBs
}

BEGIN_MESSAGE_MAP( CContrastModePage, CModePage )
	ON_EN_CHANGE( IDC_CONTRAST_PCT_EDIT, OnChange_ContrastPct )
END_MESSAGE_MAP()


// CAlphaBlendModePage implementation

CAlphaBlendModePage::CAlphaBlendModePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::AlphaBlend, IDD_IMAGE_PAGE_ALPHA_BLEND )
{
	m_channelEdits.push_back( new CColorChannelEdit( IDC_CHANNEL1_EDIT, &m_pDialog->m_sourceAlpha ) );
	m_channelEdits.push_back( new CColorChannelEdit( IDC_CHANNEL2_EDIT, &m_pDialog->m_pixelAlpha, true ) );
	m_keepEqualEdits = m_channelEdits;		// keep values in sync if checked box
}

void CAlphaBlendModePage::DoDataExchange( CDataExchange* pDX )
{
	if ( DialogOutput == pDX->m_bSaveAndValidate )
		CheckDlgButton( IDC_KEEP_EQUAL_CHECK, AfxGetApp()->GetProfileInt( reg::section_dlg, reg::entry_alphaPageKeepEqual, FALSE ) );
	else
		AfxGetApp()->WriteProfileInt( reg::section_dlg, reg::entry_alphaPageKeepEqual, IsDlgButtonChecked( IDC_KEEP_EQUAL_CHECK ) );

	__super::DoDataExchange( pDX );
}


// CBlendColorModePage implementation

CBlendColorModePage::CBlendColorModePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::BlendColor, IDD_IMAGE_PAGE_BLEND_COLOR )
{
	m_channelEdits.push_back( new CColorChannelEdit( IDC_CHANNEL1_EDIT, &m_pDialog->m_blendColor, true ) );
}

// CDisabledModePage implementation

CDisabledModePage::CDisabledModePage( CImageDialog* pDialog )
	: CModePage( pDialog, CImageDialog::Disabled, IDD_IMAGE_PAGE_DISABLED )
{
	m_channelEdits.push_back( new CColorChannelEdit( IDC_CHANNEL1_EDIT, &m_pDialog->m_blendColor ) );
	m_channelEdits.push_back( new CColorChannelEdit( IDC_CHANNEL2_EDIT, &m_pDialog->m_disabledAlpha, true ) );
}
