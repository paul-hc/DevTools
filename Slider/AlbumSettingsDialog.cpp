
#include "pch.h"
#include "AlbumSettingsDialog.h"
#include "FileAttrAlgorithms.h"
#include "ICatalogStorage.h"
#include "MainFrame.h"
#include "ImageView.h"
#include "SearchPatternDialog.h"
#include "FileOperation.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/RuntimeException.h"
#include "utl/UI/ListCtrlEditorFrame.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/Color.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Resequence.hxx"
#include "utl/UI/ReportListControl.hxx"
#include "utl/UI/DragListCtrl.hxx"


namespace hlp
{
	fattr::Order OrderOfCmd( UINT cmdId )
	{
		switch ( cmdId )
		{
			default: ASSERT( false );
			case ID_ORDER_ORIGINAL:						return fattr::OriginalOrder;
			case ID_ORDER_CUSTOM:						return fattr::CustomOrder;
			case ID_ORDER_RANDOM_SHUFFLE:				return fattr::Shuffle;
			case ID_ORDER_RANDOM_SHUFFLE_SAME_SEED:		return fattr::ShuffleSameSeed;
			case ID_ORDER_BY_FULL_PATH_ASC:				return fattr::ByFullPathAsc;
			case ID_ORDER_BY_FULL_PATH_DESC:			return fattr::ByFullPathDesc;
			case ID_ORDER_BY_DIMENSION_ASC:				return fattr::ByDimensionAsc;
			case ID_ORDER_BY_DIMENSION_DESC:			return fattr::ByDimensionDesc;
		}
	}
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_GROUP_BOX_1, SizeX },
		{ IDC_PATTERNS_LISTVIEW, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_THUMB_PREVIEW_STATIC, MoveX },
		{ IDC_DOC_VERSION_LABEL, MoveX },
		{ IDC_DOC_VERSION_STATIC, MoveX },

		{ IDC_FOUND_IMAGES_LISTVIEW, Size },
		{ IDC_STRIP_BAR_2, MoveX },
		{ IDC_LIST_ORDER_STATIC, MoveX },
		{ IDC_LIST_ORDER_COMBO, MoveX },

		{ ID_EDIT_ARCHIVE_PASSWORD, MoveY },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CAlbumSettingsDialog::CAlbumSettingsDialog( const CAlbumModel& model, size_t currentPos, CWnd* pParent /*= nullptr*/ )
	: CLayoutDialog( IDD_ALBUM_SETTINGS_DIALOG, pParent )
	, m_model( model )
	, m_pCaretFileAttr( currentPos < m_model.GetFileAttrCount() ? m_model.GetFileAttr( currentPos ) : nullptr )
	, m_isDirty( false )

	, m_patternsListCtrl( IDC_PATTERNS_LISTVIEW )
	, m_maxFileCountEdit( true, str::GetUserLocale() )
	, m_minSizeEdit( true, str::GetUserLocale() )
	, m_maxSizeEdit( true, str::GetUserLocale() )
	, m_imagesListCtrl( IDC_FOUND_IMAGES_LISTVIEW )
	, m_sortOrderCombo( &fattr::GetTags_Order() )
	, m_thumbPreviewCtrl( app::GetThumbnailer() )
	, m_docVersionLabel( CRegularStatic::ControlLabel )
	, m_docVersionStatic( CRegularStatic::Bold )
{
	// base init
	m_regSection = _T("AlbumSettingsDialog");
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	m_initCentered = false;
	LoadDlgIcon( ID_EDIT_ALBUM );
	m_accelPool.AddAccelTable( new CAccelTable( IDD_ALBUM_SETTINGS_DIALOG ) );

	m_patternsListCtrl.SetSection( m_regSection + _T("\\PatternsList") );
	m_patternsListCtrl.SetAcceptDropFiles();
	m_patternsListCtrl.SetSubjectAdapter( ui::GetFullPathAdapter() );			// display full paths
	m_patternsListCtrl.SetTextEffectCallback( this );

	m_imagesListCtrl.SetSection( m_regSection + _T("\\ImagesList") );
	m_imagesListCtrl.SetCustomImageDraw( app::GetThumbnailer() );
	m_imagesListCtrl.SetTextEffectCallback( this );
	m_imagesListCtrl.SetSortInternally( false );
	m_imagesListCtrl.SetUseAlternateRowColoring();
	m_imagesListCtrl.SetDataSourceFactory( this );								// uses temporary file clones for embedded images
	m_imagesListCtrl.SetTrackMenuTarget( this );								// let dialog track SPECIFIC custom menu commands (Explorer verbs handled by the listctrl)
	m_imagesListCtrl.SetPopupMenu( CReportListControl::OnSelection, &GetAlbumModelPopupMenu() );

	m_imagesListCtrl
		.AddTileColumn( Dimensions )
		.AddTileColumn( Size )
		.AddTileColumn( Date )
		.AddTileColumn( Folder );

	m_patternsToolbar.GetStrip()
		.AddButton( ID_ADD_ITEM )
		.AddButton( ID_REMOVE_ITEM )
		.AddButton( ID_REMOVE_ALL_ITEMS )
		.AddSeparator()
		.AddButton( ID_EDIT_ITEM )
		.AddSeparator()
		.AddButton( ID_MOVE_UP_ITEM )
		.AddButton( ID_MOVE_DOWN_ITEM )
		.AddButton( ID_MOVE_TOP_ITEM )
		.AddButton( ID_MOVE_BOTTOM_ITEM )
		.AddSeparator()
		.AddButton( ID_EDIT_COPY )
		.AddSeparator()
		.AddButton( IDC_SEARCH_FOR_FILES, ID_IMAGE_EXPLORE );

	m_imagesToolbar.GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_EDIT_SELECT_ALL )
		.AddSeparator()
		.AddButton( ID_MOVE_UP_ITEM )
		.AddButton( ID_MOVE_DOWN_ITEM )
		.AddButton( ID_MOVE_TOP_ITEM )
		.AddButton( ID_MOVE_BOTTOM_ITEM )
		.AddSeparator()
		.AddButton( ID_ORDER_ORIGINAL )
		.AddButton( ID_ORDER_CUSTOM )
		.AddButton( ID_ORDER_RANDOM_SHUFFLE )
		.AddButton( ID_ORDER_RANDOM_SHUFFLE_SAME_SEED )
		.AddButton( ID_ORDER_BY_FULL_PATH_ASC )
		.AddButton( ID_ORDER_BY_FULL_PATH_DESC )
		.AddButton( ID_ORDER_BY_DIMENSION_ASC )
		.AddButton( ID_ORDER_BY_DIMENSION_DESC )
		.AddSeparator();		// visual separator from order combo

	utl::InsertFrom( std::inserter( m_origFilePaths, m_origFilePaths.end() ), m_model.GetSearchModel()->GetPatterns(), func::AsCode() );
	utl::InsertFrom( std::inserter( m_origFilePaths, m_origFilePaths.end() ), m_model.GetImagesModel().GetFileAttrs(), func::ToFilePath() );
}

CAlbumSettingsDialog::~CAlbumSettingsDialog()
{
}

int CAlbumSettingsDialog::GetCurrentIndex( void ) const
{
	if ( nullptr == m_pCaretFileAttr )
		return m_model.AnyFoundFiles() ? 0 : -1;

	return static_cast<int>( utl::FindPos( m_model.GetImagesModel().GetFileAttrs(), m_pCaretFileAttr ) );
}

CMenu& CAlbumSettingsDialog::GetAlbumModelPopupMenu( void )
{
	static CMenu s_popupMenu;
	if ( nullptr == s_popupMenu.GetSafeHmenu() )
	{
		CMenu popupMenu;
		ui::LoadPopupMenu( &s_popupMenu, IDR_CONTEXT_MENU, app::AlbumFoundListPopup );
		ui::JoinMenuItems( &s_popupMenu, &CPathItemListCtrl::GetStdPathListPopupMenu( CReportListControl::OnSelection ) );
	}
	return s_popupMenu;
}

ole::CDataSource* CAlbumSettingsDialog::NewDataSource( void )
{
	return new ole::CImagesDataSource();
}

bool CAlbumSettingsDialog::SetDirty( bool dirty /*= true*/ )
{
	if ( IsInternalChange() )
		return false;			// during dialog output

	m_isDirty = dirty;
	m_imagesListCtrl.Invalidate();
	m_imagesListCtrl.UpdateWindow();

	static const struct { std::tstring m_caption; UINT m_iconId; } s_okLabelIcon[] = { { _T("OK"), 0 }, { _T("&Search"), ID_IMAGE_EXPLORE } };
	m_okButton.SetButtonCaption( s_okLabelIcon[ m_isDirty ].m_caption );
	m_okButton.SetIconId( s_okLabelIcon[ m_isDirty ].m_iconId );

	ui::EnableControl( m_hWnd, IDOK, m_isDirty || m_model.GetFileOrder() != fattr::FilterCorruptedFiles );
	return true;
}

void CAlbumSettingsDialog::SetupPatternsListView( void )
{
	CScopedInternalChange internalChange( &m_patternsListCtrl );
	CScopedLockRedraw freeze( &m_patternsListCtrl );

	// fill in the found files list (File Name|In Folder|Size|Date)
	m_patternsListCtrl.DeleteAllItems();

	const std::vector<CSearchPattern*>& patterns = m_model.GetSearchModel()->GetPatterns();

	for ( UINT i = 0, count = static_cast<UINT>( patterns.size() ); i != count; ++i )
	{
		CSearchPattern* pPattern = patterns[ i ];

		m_patternsListCtrl.InsertObjectItem( i, pPattern, ui::Transparent_Image );
		m_patternsListCtrl.SetSubItemText( i, PatternType, CSearchPattern::GetTags_Type().FormatUi( pPattern->GetType() ) );
		m_patternsListCtrl.SetSubItemText( i, PatternDepth, CSearchPattern::GetTags_SearchMode().FormatUi( pPattern->GetSearchMode() ) );
	}
}

void CAlbumSettingsDialog::SetupFoundImagesListView( void )
{
	CWaitCursor wait;
	lv::TScopedStatus_ByText scopedSel( &m_imagesListCtrl );

	size_t count = m_model.GetFileAttrCount();
	{
		CScopedInternalChange internalChange( &m_imagesListCtrl );
		CScopedLockRedraw freeze( &m_imagesListCtrl );

		// fill in the found files list (File Name|In Folder|Size|Date)
		m_imagesListCtrl.DeleteAllItems();

		for ( UINT i = 0; i != count; ++i )
		{
			const CFileAttr* pFileAttr = m_model.GetFileAttr( i );

			m_imagesListCtrl.InsertObjectItem( i, const_cast<CFileAttr*>( pFileAttr ), ui::Transparent_Image );
			m_imagesListCtrl.SetSubItemText( i, Folder, pFileAttr->GetPath().GetOriginParentPath().Get() );
			m_imagesListCtrl.SetItemText( i, Dimensions, LPSTR_TEXTCALLBACK );			// defer CPU-intensive dimensions evaluation

			std::tstring sizeText;
			switch ( m_model.GetFileOrder() )
			{
				case fattr::FilterFileSameSize:			sizeText = pFileAttr->FormatFileSize( 1, _T("%s B") ); break;
				case fattr::FilterFileSameSizeAndDim:	sizeText = pFileAttr->FormatFileSize( 1, _T("%s B (%dx%d)") ); break;
				default:								sizeText = pFileAttr->FormatFileSize();
			}

			m_imagesListCtrl.SetSubItemText( i, Size, sizeText );
			m_imagesListCtrl.SetSubItemText( i, Date, pFileAttr->FormatLastModifTime() );
		}
	}

	ui::SetDlgItemText( m_hWnd, IDC_FOUND_FILES_STATIC, str::Format( _T("&Found %d file(s):"), count ) );
}

void CAlbumSettingsDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem;
	static const ui::CTextEffect s_errorEffect( ui::Regular, app::ColorErrorText, app::ColorErrorBk );
	static const ui::CTextEffect s_newFileEffect( ui::Regular, color::Blue );
	static const ui::CTextEffect s_stgFileEffect( ui::Underline );

	if ( pCtrl == &m_patternsListCtrl )
	{
		const CSearchPattern* pPattern = CReportListControl::AsPtr<CSearchPattern>( rowKey );

		if ( PatternPath == subItem && pPattern->IsStorageAlbumFile() )
			rTextEffect |= s_stgFileEffect;						// highlight storage item

		if ( IsNewFilePath( pPattern->GetFilePath() ) )
			rTextEffect |= s_newFileEffect;						// highlight new file item

		if ( !pPattern->IsValidPath() )
			rTextEffect |= s_errorEffect;						// highlight error item
	}
	else if ( pCtrl == &m_imagesListCtrl )
	{
		const CFileAttr* pFileAttr = CReportListControl::AsPtr<CFileAttr>( rowKey );

		if ( Folder == subItem && CCatalogStorageFactory::HasCatalogExt( pFileAttr->GetPath().GetOriginParentPath().GetPtr() ) )
			rTextEffect |= s_stgFileEffect;						// highlight storage item

		if ( IsNewFilePath( pFileAttr->GetPath() ) )
			rTextEffect |= s_newFileEffect;

		if ( !pFileAttr->IsValid() )
			rTextEffect |= s_errorEffect;

		if ( m_isDirty )
		{
			if ( CLR_NONE == rTextEffect.m_textColor )
				rTextEffect.m_textColor = m_imagesListCtrl.GetActualTextColor();

			if ( CLR_DEFAULT == rTextEffect.m_textColor )
				rTextEffect.m_textColor = ::GetSysColor( COLOR_WINDOWTEXT );

			rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor, color::White );		// blend to washed out gray effect
		}
	}
}

std::pair<CAlbumSettingsDialog::ImagesColumn, bool> CAlbumSettingsDialog::ToListSortOrder( fattr::Order fileOrder )
{
	switch ( fileOrder )
	{
		case fattr::ByFileNameAsc:		return std::make_pair( FileName, true );
		case fattr::ByFileNameDesc:		return std::make_pair( FileName, false ); break;
		case fattr::ByFullPathAsc:		return std::make_pair( Folder, true ); break;
		case fattr::ByFullPathDesc:		return std::make_pair( Folder, false ); break;
		case fattr::ByDimensionAsc:		return std::make_pair( Dimensions, true ); break;
		case fattr::ByDimensionDesc:	return std::make_pair( Dimensions, false ); break;
		case fattr::BySizeAsc:			return std::make_pair( Size, true ); break;
		case fattr::BySizeDesc:			return std::make_pair( Size, false ); break;
		case fattr::ByDateAsc:			return std::make_pair( Date, true ); break;
		case fattr::ByDateDesc:			return std::make_pair( Date, false ); break;
	}
	return std::make_pair( Unordered, false );
}

bool CAlbumSettingsDialog::SearchSourceFiles( void )
{
	CWaitCursor wait;
	bool success = true;

	ui::EnableControl( m_hWnd, IDCANCEL, false );

	fs::CFlexPath currFilePath;
	if ( m_pCaretFileAttr != nullptr )
		currFilePath = m_pCaretFileAttr->GetPath();

	try
	{
		m_model.SearchForFiles( this );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		success = false;
	}

	if ( !currFilePath.IsEmpty() )
		m_pCaretFileAttr = fattr::FindWithPath( m_model.GetImagesModel().GetFileAttrs(), currFilePath );
	else
		m_pCaretFileAttr = nullptr;

	SetupFoundImagesListView();		// fill in the found files list
	SetDirty( false );
	ui::EnableControl( m_hWnd, IDCANCEL );
	return success;
}

void CAlbumSettingsDialog::UpdateCurrentFile( void )
{
	fs::CFlexPath currFilePath;
	int selCaretIndex = m_imagesListCtrl.GetSelCaretIndex();

	if ( selCaretIndex != -1 )
		currFilePath = m_imagesListCtrl.GetObjectAt<CFileAttr>( selCaretIndex )->GetPath();

	m_thumbPreviewCtrl.SetImagePath( currFilePath );
}

int CAlbumSettingsDialog::GetCheckStateAutoRegen( void ) const
{
	if ( m_model.IsAutoDropRecipient() )
		return BST_INDETERMINATE;

	return m_model.HasPersistFlag( CAlbumModel::AutoRegenerate ) ? BST_CHECKED : BST_UNCHECKED;
}

void CAlbumSettingsDialog::OutputAll( void )
{
	CScopedInternalChange scopedDisableInput( this );

	m_docVersionStatic.SetWindowText( app::FormatModelVersion( m_model.GetModelSchema() ) );

	SetupPatternsListView();

	const CSearchModel* pSearchModel = m_model.GetSearchModel();
	CheckDlgButton( IDC_MAX_FILE_COUNT_CHECK, pSearchModel->GetMaxFileCount() != UINT_MAX );
	CheckDlgButton( IDC_MIN_FILE_SIZE_CHECK, pSearchModel->GetFileSizeRange().m_start != CSearchModel::s_anyFileSizeRange.m_start );
	CheckDlgButton( IDC_MAX_FILE_SIZE_CHECK, pSearchModel->GetFileSizeRange().m_end != CSearchModel::s_anyFileSizeRange.m_end );
	OnToggle_MaxFileCount();
	OnToggle_MinSize();
	OnToggle_MaxSize();

	// auto-drop side effect: also auto regenerate, but disable the check
	CheckDlgButton( IDC_AUTO_REGENERATE_CHECK, GetCheckStateAutoRegen() );
	CheckDlgButton( IDC_AUTO_DROP_CHECK, m_model.IsAutoDropRecipient() );

	// fill in the search attribute path list
	m_sortOrderCombo.SetValue( m_model.GetFileOrder() );

	m_patternsListCtrl.SetCurSel( 0 );

	// setup list-control
	std::pair<ImagesColumn, bool> sortPair = ToListSortOrder( m_model.GetFileOrder() );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

	SetupFoundImagesListView();

	if ( !m_imagesListCtrl.AnySelected() )
		if ( m_pCaretFileAttr != nullptr )
			m_imagesListCtrl.Select( m_pCaretFileAttr );

	m_thumbPreviewCtrl.SetImagePath( m_pCaretFileAttr != nullptr ? m_pCaretFileAttr->GetPath() : fs::CFlexPath() );
}

void CAlbumSettingsDialog::InputAll( void )
{
	CSearchModel* pSearchModel = m_model.RefSearchModel();
	UINT maxFileCount = UINT_MAX;

	if ( IsDlgButtonChecked( IDC_MAX_FILE_COUNT_CHECK ) )
		if ( !m_maxFileCountEdit.ParseNumber( &maxFileCount ) )
			maxFileCount = UINT_MAX;

	pSearchModel->SetMaxFileCount( maxFileCount );

	Range<UINT> fileSizeRange = CSearchModel::s_anyFileSizeRange;

	if ( IsDlgButtonChecked( IDC_MIN_FILE_SIZE_CHECK ) )
		if ( m_minSizeEdit.ParseNumber( &fileSizeRange.m_start ) )
			fileSizeRange.m_start *= KiloByte;

	if ( IsDlgButtonChecked( IDC_MAX_FILE_SIZE_CHECK ) )
		if ( m_maxSizeEdit.ParseNumber( &fileSizeRange.m_end ) )
			fileSizeRange.m_end *= KiloByte;

	pSearchModel->SetFileSizeRange( fileSizeRange );

	UINT ckState = IsDlgButtonChecked( IDC_AUTO_REGENERATE_CHECK );
	if ( ckState != BST_INDETERMINATE )
		m_model.SetPersistFlag( CAlbumModel::AutoRegenerate, BST_CHECKED == ckState );		// the button is checked naturally, and not as a side effect of auto-drop feature

	m_model.StoreFileOrder( m_sortOrderCombo.GetEnum<fattr::Order>() );
	m_pCaretFileAttr = m_imagesListCtrl.GetSelected<CFileAttr>();
}

void CAlbumSettingsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_imagesListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_PATTERNS_LISTVIEW, m_patternsListCtrl );
	m_patternsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignRight | V_AlignCenter );

	DDX_Control( pDX, IDC_MAX_FILE_COUNT_EDIT, m_maxFileCountEdit );
	DDX_Control( pDX, IDC_MIN_FILE_SIZE_EDIT, m_minSizeEdit );
	DDX_Control( pDX, IDC_MAX_FILE_SIZE_EDIT, m_maxSizeEdit );

	DDX_Control( pDX, IDC_LIST_ORDER_COMBO, m_sortOrderCombo );
	DDX_Control( pDX, IDC_FOUND_IMAGES_LISTVIEW, m_imagesListCtrl );
	m_imagesToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignRight | V_AlignCenter );

	DDX_Control( pDX, IDC_THUMB_PREVIEW_STATIC, m_thumbPreviewCtrl );
	DDX_Control( pDX, IDC_DOC_VERSION_LABEL, m_docVersionLabel );
	DDX_Control( pDX, IDC_DOC_VERSION_STATIC, m_docVersionStatic );
	DDX_Control( pDX, IDOK, m_okButton );
	ui::DDX_ButtonIcon( pDX, ID_EDIT_ARCHIVE_PASSWORD, /*ID_EDIT_PASTE*/ IDD_PASSWORD_DIALOG );		// decorate with icon: doesn't work in Slider, but works fine in DemoUtl.exe

	if ( firstInit )
	{
		m_pPatternsEditor.reset( new CListCtrlEditorFrame( &m_patternsListCtrl, &m_patternsToolbar ) );
		m_pImagesEditor.reset( new CListCtrlEditorFrame( &m_imagesListCtrl, &m_imagesToolbar ) );

		if ( m_model.GetCatalogStorage() != nullptr )
			m_patternsListCtrl.EnableWindow( false );		// disable search patterns editing for catalog-based albums

		m_imagesListCtrl.SetCompactIconSpacing();

		CAccelTable* pImagesAccel = m_pImagesEditor->RefListAccel();
		CAccelKeys imagesKeys;
		pImagesAccel->QueryKeys( imagesKeys.m_keys );
		imagesKeys.ReplaceCmdId( ID_REMOVE_ITEM, ID_IMAGE_DELETE );
		pImagesAccel->Create( imagesKeys.m_keys );
	}

	switch ( pDX->m_bSaveAndValidate )
	{
		case DialogOutput:
			OutputAll();
			SetDirty( m_model.MustAutoRegenerate() );		// set initial dirty if auto-generation is turned on (either explicit or by auto-drop)
			break;
		case DialogSaveChanges:
			InputAll();
	}

	__super::DoDataExchange( pDX );
}

BOOL CAlbumSettingsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	// NOTE: no routing to the patterns or images lists - it works the other way around, read the notes in CReportListControl::OnCmdMsg().

	switch ( id )
	{
		case ID_EDIT_COPY:
			return FALSE;		// special commands: prevent base handling since command gets wrongly handled in CImageView::OnEditCopy (via dialog's parent routing and MFC default routing)
	}

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BOOL CAlbumSettingsDialog::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( m_pPatternsEditor->HandleTranslateMessage( pMsg ) )
			return true;
		else if ( m_pImagesEditor->HandleTranslateMessage( pMsg ) )
			return true;

	return __super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumSettingsDialog, CLayoutDialog )
	ON_NOTIFY( NM_DBLCLK, IDC_PATTERNS_LISTVIEW, OnLVnDblClk_Patterns )
	ON_NOTIFY( lv::LVN_DropFiles, IDC_PATTERNS_LISTVIEW, OnLVnDropFiles_Patterns )
	ON_NOTIFY( lv::LVN_ItemsRemoved, IDC_PATTERNS_LISTVIEW, OnLVnItemsRemoved_Patterns )
	ON_CONTROL( lv::LVN_ItemsReorder, IDC_PATTERNS_LISTVIEW, OnLVnItemsReorder_Patterns )
	ON_COMMAND( ID_ADD_ITEM, On_AddSearchPattern )			// validated by m_patternsEditor
	ON_COMMAND( ID_EDIT_ITEM, On_ModifySearchPattern )		// validated by m_patternsEditor
	ON_COMMAND( IDC_SEARCH_FOR_FILES, OnSearchSourceFiles )

	ON_BN_CLICKED( IDC_MAX_FILE_COUNT_CHECK, OnToggle_MaxFileCount )
	ON_BN_CLICKED( IDC_MIN_FILE_SIZE_CHECK, OnToggle_MinSize )
	ON_BN_CLICKED( IDC_MAX_FILE_SIZE_CHECK, OnToggle_MaxSize )
	ON_EN_CHANGE( IDC_MAX_FILE_COUNT_EDIT, OnEnChange_MaxFileCount )
	ON_EN_CHANGE( IDC_MIN_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_EN_CHANGE( IDC_MAX_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_BN_CLICKED( IDC_AUTO_REGENERATE_CHECK, OnToggle_AutoRegenerate )
	ON_BN_CLICKED( IDC_AUTO_DROP_CHECK, OnToggle_AutoDrop )

	ON_NOTIFY( LVN_COLUMNCLICK, IDC_FOUND_IMAGES_LISTVIEW, OnLVnColumnClick_FoundImages )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FOUND_IMAGES_LISTVIEW, OnLVnItemChanged_FoundImages )
	ON_NOTIFY( LVN_GETDISPINFO, IDC_FOUND_IMAGES_LISTVIEW, OnLVnGetDispInfo_FoundImages )
	ON_CONTROL( lv::LVN_ItemsReorder, IDC_FOUND_IMAGES_LISTVIEW, OnLVnItemsReorder_FoundImages )
	ON_STN_DBLCLK( IDC_THUMB_PREVIEW_STATIC, OnStnDblClk_ThumbPreviewStatic )

	ON_COMMAND_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, OnImageOrder )
	ON_UPDATE_COMMAND_UI_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, OnUpdateImageOrder )
	ON_CBN_SELCHANGE( IDC_LIST_ORDER_COMBO, OnCBnSelChange_ImageOrder )

	// image file operations
	ON_COMMAND( ID_IMAGE_OPEN, On_ImageOpen )
	ON_COMMAND( ID_IMAGE_SAVE_AS, On_ImageSaveAs )
	ON_COMMAND( ID_IMAGE_DELETE, On_ImageRemove )
	ON_COMMAND( ID_IMAGE_EXPLORE, On_ImageExplore )
	ON_UPDATE_COMMAND_UI_RANGE( ID_IMAGE_OPEN, ID_IMAGE_EXPLORE, OnUpdate_ImageFileOp )
END_MESSAGE_MAP()

void CAlbumSettingsDialog::OnOK( void )
{
	if ( UpdateData( DialogSaveChanges ) )
		if ( m_isDirty )
			SearchSourceFiles();
		else
			__super::OnOK();
}

void CAlbumSettingsDialog::OnIdleUpdateControls( void )
{
	__super::OnIdleUpdateControls();

	ui::UpdateDlgItemUI( this, ID_EDIT_ARCHIVE_PASSWORD );		// update button icon for password protected state
}

void CAlbumSettingsDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( pWnd == &m_imagesListCtrl )
	{
		if ( m_imagesListCtrl.GetSelectedCount() != 0 )
			ui::TrackContextMenu( IDR_CONTEXT_MENU, app::AlbumFoundListPopup, this, screenPos );
	}
	else
		__super::OnContextMenu( pWnd, screenPos );
}


void CAlbumSettingsDialog::OnLVnDblClk_Patterns( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMITEMACTIVATE* pNmItemActivate = (NMITEMACTIVATE*)pNmHdr;
	*pResult = 0;

	if ( pNmItemActivate->iItem != -1 )
		ui::SendCommand( m_hWnd, ID_EDIT_ITEM, BN_CLICKED, pNmItemActivate->hdr.hwndFrom );
}

void CAlbumSettingsDialog::OnLVnDropFiles_Patterns( NMHDR* pNmHdr, LRESULT* pResult )
{
	const lv::CNmDropFiles* pNmDropFiles = (const lv::CNmDropFiles*)pNmHdr;
	*pResult = 0;

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	std::vector<CSearchPattern*> droppedPatterns;

	size_t dropPos = pNmDropFiles->m_dropItemIndex;
	for ( std::vector<fs::CPath>::const_iterator itSearchPath = pNmDropFiles->m_filePaths.begin(); itSearchPath != pNmDropFiles->m_filePaths.end(); ++itSearchPath )
	{
		std::pair<CSearchPattern*, bool> patternPair = pSearchModel->AddSearchPath( *itSearchPath, dropPos );
		if ( patternPair.first != nullptr )
		{
			droppedPatterns.push_back( patternPair.first );

			if ( patternPair.second )
				++dropPos;
		}
	}

	SetupPatternsListView();
	m_patternsListCtrl.SelectObjects( droppedPatterns );
	SetDirty();
}

void CAlbumSettingsDialog::OnLVnItemsRemoved_Patterns( NMHDR* pNmHdr, LRESULT* pResult )
{
	const lv::CNmItemsRemoved* pNmItemsRemoved = (const lv::CNmItemsRemoved*)pNmHdr;
	*pResult = 0;

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	std::vector<CSearchPattern*>& rSearchPatterns = pSearchModel->RefPatterns();

	if ( pNmItemsRemoved->m_removedObjects.size() == rSearchPatterns.size() )		// remove all?
		pSearchModel->ClearPatterns();
	else
		for ( std::vector<utl::ISubject*>::const_iterator itObject = pNmItemsRemoved->m_removedObjects.begin(); itObject != pNmItemsRemoved->m_removedObjects.end(); ++itObject )
		{
			CSearchPattern* pPattern = checked_static_cast<CSearchPattern*>( *itObject );

			utl::RemoveExisting( rSearchPatterns, pPattern );
			delete pPattern;
		}

	SetDirty();
}

void CAlbumSettingsDialog::OnLVnItemsReorder_Patterns( void )
{
	// copy List sequence -> CSearchModel sequence
	std::vector<CSearchPattern*> patternsSequence;
	m_patternsListCtrl.QueryObjectsSequence( patternsSequence );

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	ASSERT( utl::SameContents( patternsSequence, pSearchModel->GetPatterns() ) );

	pSearchModel->RefPatterns().swap( patternsSequence );
	SetDirty();
}

void CAlbumSettingsDialog::On_AddSearchPattern( void )
{
	CSearchPattern* pCaretPattern = m_patternsListCtrl.GetCaretAs<CSearchPattern>();
	CSearchPatternDialog dlg( pCaretPattern, this );
	if ( dlg.DoModal() != IDOK )
		return;

	CSearchModel* pSearchModel = m_model.RefSearchModel();

	size_t pos = pSearchModel->FindPatternPos( dlg.m_pSearchPattern->GetFilePath() );
	if ( pos != utl::npos )
		*pSearchModel->GetPatternAt( pos ) = *dlg.m_pSearchPattern;				// modify existing pattern
	else
		pos = pSearchModel->AddPattern( dlg.m_pSearchPattern.release() );		// insert new pattern

	SetupPatternsListView();
	m_patternsListCtrl.SetCurSel( static_cast<int>( pos ) );
	GotoDlgCtrl( &m_patternsListCtrl );
	SetDirty();
}

void CAlbumSettingsDialog::On_ModifySearchPattern( void )
{
	size_t selIndex = m_patternsListCtrl.GetCurSel();
	ASSERT( selIndex != utl::npos );		// should be validated

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	std::vector<CSearchPattern*>& rSearchPatterns = pSearchModel->RefPatterns();

	CSearchPattern* pEditedPattern = rSearchPatterns[ selIndex ];

	CSearchPatternDialog dlg( pEditedPattern, this );
	if ( dlg.DoModal() != IDOK )
		return;

	size_t dupPos = pSearchModel->FindPatternPos( dlg.m_pSearchPattern->GetFilePath(), selIndex );
	if ( dupPos != utl::npos )
	{
		m_patternsListCtrl.SetCaretIndex( static_cast<int>( dupPos ) );
		ui::MessageBox( str::Format( _T("The search pattern must be unique!\nCheck duplicate pattern no. %d."), dupPos + 1 ), MB_ICONERROR | MB_OK );
		return;
	}

	*pEditedPattern = *dlg.m_pSearchPattern;

	SetupPatternsListView();
	m_patternsListCtrl.SetCurSel( static_cast<int>( selIndex ) );
	GotoDlgCtrl( &m_patternsListCtrl );
	SetDirty();
}

void CAlbumSettingsDialog::OnSearchSourceFiles( void )
{
	SearchSourceFiles();
}


void CAlbumSettingsDialog::OnToggle_MaxFileCount( void )
{
	if ( IsDlgButtonChecked( IDC_MAX_FILE_COUNT_CHECK ) )
	{
		UINT value = m_model.GetSearchModel()->GetMaxFileCount();
		if ( UINT_MAX == value )
			value = 1000;

		ui::EnableWindow( m_maxFileCountEdit );
		m_maxFileCountEdit.SetNumber( value );
	}
	else
	{
		ui::EnableWindow( m_maxFileCountEdit, false );
		ui::SetWindowText( m_maxFileCountEdit, std::tstring() );
	}
	SetDirty();
}

void CAlbumSettingsDialog::OnToggle_MinSize( void )
{
	if ( IsDlgButtonChecked( IDC_MIN_FILE_SIZE_CHECK ) )
	{
		ui::EnableWindow( m_minSizeEdit );
		m_minSizeEdit.SetNumber( m_model.GetSearchModel()->GetFileSizeRange().m_start / KiloByte );
	}
	else
	{
		ui::EnableWindow( m_minSizeEdit, false );
		ui::SetWindowText( m_minSizeEdit, std::tstring() );
	}
	SetDirty();
}

void CAlbumSettingsDialog::OnToggle_MaxSize( void )
{
	if ( IsDlgButtonChecked( IDC_MAX_FILE_SIZE_CHECK ) )
	{
		UINT value = m_model.GetSearchModel()->GetFileSizeRange().m_end;
		if ( UINT_MAX == value )
			value = MegaByte / 2;

		ui::EnableWindow( m_maxSizeEdit );
		m_maxSizeEdit.SetNumber( value / KiloByte );
	}
	else
	{
		ui::EnableWindow( m_maxSizeEdit, false );
		ui::SetWindowText( m_maxSizeEdit, std::tstring() );
	}
	SetDirty();
}

void CAlbumSettingsDialog::OnEnChange_MaxFileCount( void )
{
	SetDirty();
}

void CAlbumSettingsDialog::OnEnChange_MinMaxSize( void )
{
	SetDirty();
}

void CAlbumSettingsDialog::OnToggle_AutoRegenerate( void )
{
	UINT ckState = IsDlgButtonChecked( IDC_AUTO_REGENERATE_CHECK );
	if ( ckState != BST_INDETERMINATE )
		m_model.SetPersistFlag( CAlbumModel::AutoRegenerate, BST_CHECKED == ckState );		// the button is checked naturally, and not as a side effect of auto-drop feature
	SetDirty();
}

void CAlbumSettingsDialog::OnToggle_AutoDrop( void )
{
	AfxMessageBox( IDS_NO_DIR_SEARCH_SPEC );
}


void CAlbumSettingsDialog::OnLVnColumnClick_FoundImages( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	fattr::Order fileOrder = m_model.GetFileOrder();

	switch ( pNmList->iSubItem )
	{
		case FileName:
			fileOrder = fileOrder == fattr::ByFileNameAsc ? fattr::ByFileNameDesc : fattr::ByFileNameAsc;
			break;
		case Folder:
			fileOrder = fileOrder == fattr::ByFullPathAsc ? fattr::ByFullPathDesc : fattr::ByFullPathAsc;
			break;
		case Dimensions:
			fileOrder = fileOrder == fattr::ByDimensionAsc ? fattr::ByDimensionDesc : fattr::ByDimensionAsc;
			break;
		case Size:
			fileOrder = fileOrder == fattr::BySizeAsc ? fattr::BySizeDesc : fattr::BySizeAsc;
			break;
		case Date:
			fileOrder = fileOrder == fattr::ByDateAsc ? fattr::ByDateDesc : fattr::ByDateAsc;
			break;
	}

	m_sortOrderCombo.SetValue( fileOrder );
	m_model.ModifyFileOrder( fileOrder );
	SetupFoundImagesListView();

	*pResult = 0;
}

void CAlbumSettingsDialog::OnLVnItemChanged_FoundImages( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		UpdateCurrentFile();
}

void CAlbumSettingsDialog::OnLVnGetDispInfo_FoundImages( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDisplayInfo = (LV_DISPINFO*)pNmHdr;

	if ( HasFlag( pDisplayInfo->item.mask, LVIF_TEXT ) )
		switch ( pDisplayInfo->item.iSubItem )
		{
			case Dimensions:
			{
				const CFileAttr* pFileAttr = CReportListControl::AsPtr<CFileAttr>( pDisplayInfo->item.lParam );
				const CSize& dim = pFileAttr->GetImageDim();
				std::tstring text = str::Format( _T("%dx%d"), dim.cx, dim.cy );
				_tcsncpy( pDisplayInfo->item.pszText, text.c_str(), pDisplayInfo->item.cchTextMax );
				break;
			}
		}

	*pResult = 0;
}

void CAlbumSettingsDialog::OnLVnItemsReorder_FoundImages( void )
{
	// input custom order
	std::vector<CFileAttr*> customSequence;
	m_imagesListCtrl.QueryObjectsSequence( customSequence );

	ASSERT( utl::SameContents( customSequence, m_model.GetImagesModel().GetFileAttrs() ) );

	m_model.SetCustomOrderSequence( customSequence );

	// update UI
	fattr::Order fileOrder = m_model.GetFileOrder();
	m_sortOrderCombo.SetValue( fileOrder );
	std::pair<ImagesColumn, bool> sortPair = ToListSortOrder( fileOrder );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );
}

void CAlbumSettingsDialog::OnStnDblClk_ThumbPreviewStatic( void )
{
	shell::Execute( this, m_thumbPreviewCtrl.GetImagePath().GetPtr() );
}

void CAlbumSettingsDialog::OnImageOrder( UINT cmdId )
{
	fattr::Order fileOrder = hlp::OrderOfCmd( cmdId );

	m_sortOrderCombo.SetValue( fileOrder );
	m_model.ModifyFileOrder( fileOrder );
	SetupFoundImagesListView();
}

void CAlbumSettingsDialog::OnUpdateImageOrder( CCmdUI* pCmdUI )
{
	fattr::Order fileOrder = hlp::OrderOfCmd( pCmdUI->m_nID );

	pCmdUI->SetCheck( fileOrder == m_model.GetFileOrder() );
}

void CAlbumSettingsDialog::OnCBnSelChange_ImageOrder( void )
{
	fattr::Order selFileOrder = m_sortOrderCombo.GetEnum<fattr::Order>();

	std::pair<ImagesColumn, bool> sortPair = ToListSortOrder( selFileOrder );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

	m_model.ModifyFileOrder( selFileOrder );
	SetupFoundImagesListView();
}

void CAlbumSettingsDialog::On_ImageOpen( void )
{
	std::vector<fs::CFlexPath> selImagePaths;
	m_imagesListCtrl.QuerySelectedItemPaths( selImagePaths );

	for ( std::vector<fs::CFlexPath>::const_iterator itImagePath = selImagePaths.begin(); itImagePath != selImagePaths.end(); ++itImagePath )
		AfxGetApp()->OpenDocumentFile( itImagePath->GetPtr() );
}

void CAlbumSettingsDialog::On_ImageSaveAs( void )
{
	std::vector<fs::CFlexPath> selImagePaths;
	if ( !m_imagesListCtrl.QuerySelectedItemPaths( selImagePaths ) )
		return;

	std::vector<fs::CPath> destFilePaths;
	if ( svc::PickDestImagePaths( destFilePaths, selImagePaths ) )
	{
		CFileOperation fileOp;

		for ( size_t i = 0; i != destFilePaths.size(); ++i )
			fileOp.Copy( selImagePaths[ i ], fs::CastFlexPath( destFilePaths[ i ] ) );
	}
}

void CAlbumSettingsDialog::On_ImageRemove( void )
{
	std::vector<fs::CFlexPath> selImagePaths;
	m_imagesListCtrl.QuerySelectedItemPaths( selImagePaths );

	m_imagesListCtrl.DeleteSelection();
	m_model.DeleteFromAlbum( selImagePaths );

	SetDirty( false );			// allow changes to be applied: "OK"
}

void CAlbumSettingsDialog::On_ImageExplore( void )
{
	const CFileAttr* pCurrFileAttr = m_imagesListCtrl.GetSelected<CFileAttr>();
	ASSERT_PTR( pCurrFileAttr );
	shell::ExploreAndSelectFile( pCurrFileAttr->GetPath().GetPhysicalPath().GetPtr() );
}

void CAlbumSettingsDialog::OnUpdate_ImageFileOp( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case ID_IMAGE_OPEN:
		case ID_IMAGE_SAVE_AS:
		case ID_IMAGE_DELETE:
			pCmdUI->Enable( m_imagesListCtrl.AnySelected() );
			break;
		case ID_IMAGE_EXPLORE:
		{
			const CFileAttr* pCurrFileAttr = m_imagesListCtrl.GetSelected<CFileAttr>();
			pCmdUI->Enable( pCurrFileAttr != nullptr && pCurrFileAttr->IsValid() );
			break;
		}
		default:
			pCmdUI->Enable( false );
	}
}
