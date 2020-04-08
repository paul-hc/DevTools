
#include "stdafx.h"
#include "AlbumSettingsDialog.h"
#include "MainFrame.h"
#include "ImageView.h"
#include "SearchSpecDialog.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/UI/Color.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"
#include "utl/UI/DragListCtrl.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	CFileList::Order OrderOfCmd( UINT cmdId )
	{
		switch ( cmdId )
		{
			default: ASSERT( false );
			case ID_ORDER_ORIGINAL:						return CFileList::OriginalOrder;
			case ID_ORDER_CUSTOM:						return CFileList::CustomOrder;
			case ID_ORDER_RANDOM_SHUFFLE:				return CFileList::Shuffle;
			case ID_ORDER_RANDOM_SHUFFLE_SAME_SEED:		return CFileList::ShuffleSameSeed;
			case ID_ORDER_BY_FULL_PATH_ASC:				return CFileList::ByFullPathAsc;
			case ID_ORDER_BY_FULL_PATH_DESC:			return CFileList::ByFullPathDesc;
			case ID_ORDER_BY_DIMENSION_ASC:				return CFileList::ByDimensionAsc;
			case ID_ORDER_BY_DIMENSION_DESC:			return CFileList::ByDimensionDesc;
		}
	}
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_SEARCH_SPEC_STATIC, SizeX | DoRepaint },
		{ IDC_SEARCH_SPEC_LIST, SizeX },
		{ CM_ADD_SEARCH_SPEC, None },
		{ CM_MODIFY_SEARCH_SPEC, None },
		{ CM_DELETE_SEARCH_SPEC, None },
		{ CM_MOVE_UP_SEARCH_SPEC, MoveX },
		{ CM_MOVE_DOWN_SEARCH_SPEC, MoveX },
		{ IDC_MIN_FILE_SIZE_CHECK, None },
		{ IDC_MIN_FILE_SIZE_EDIT, None },
		{ IDC_MAX_FILE_SIZE_CHECK, None },
		{ IDC_MAX_FILE_SIZE_EDIT, None },

		{ IDC_AUTO_REGENERATE_CHECK, None },
		{ IDC_AUTO_DROP_CHECK, None },

		{ IDC_FOUND_FILES_GAP, SizeX | DoRepaint },

		{ IDC_FOUND_FILES_STATIC, None },
		{ IDC_FOUND_FILES_LISTVIEW, Size },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX },
		{ IDC_LIST_ORDER_STATIC, MoveX },
		{ IDC_LIST_ORDER_COMBO, MoveX },

		{ IDOK, Move },
		{ IDCANCEL, Move },
		{ IDC_THUMB_PREVIEW_STATIC, MoveX }
	};
}


CAlbumSettingsDialog::CAlbumSettingsDialog( const CFileList& fileList, int currentIndex /*= -1*/, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_ALBUM_SETTINGS_DIALOG, pParent )
	, m_fileList( fileList )
	, m_currentIndex( currentIndex )
	, m_dlgAccel( IDR_ALBUM_DLG_ACCEL )
	, m_searchListAccel( IDR_ALBUM_DLG_SEARCH_SPEC_ACCEL )
	, m_isDirty( Undefined )
	, m_thumbPreviewCtrl( app::GetThumbnailer() )
	, m_foundFilesListCtrl( IDC_FOUND_FILES_LISTVIEW )
{
	// base init
	m_regSection = _T("AlbumSettingsDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	m_initCentered = false;
	LoadDlgIcon( CM_EDIT_ALBUM );

	m_toolbar.GetStrip()
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
		.AddSeparator();

	m_foundFilesListCtrl.SetSection( m_regSection + _T("\\List") );
	m_foundFilesListCtrl.SetCustomImageDraw( app::GetThumbnailer() );
	m_foundFilesListCtrl.SetTextEffectCallback( this );
	m_foundFilesListCtrl.SetSortInternally( false );
	m_foundFilesListCtrl.SetUseAlternateRowColoring();
	m_foundFilesListCtrl.SetDataSourceFactory( this );						// uses temporary file clones for embedded images
	m_foundFilesListCtrl.SetPopupMenu( CReportListControl::OnSelection, &GetFileListPopupMenu() );
	m_foundFilesListCtrl.SetTrackMenuTarget( this );						// let dialog track SPECIFIC custom menu commands (Explorer verbs handled by the listctrl)

	ClearFlag( m_foundFilesListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER );		// better looking thumb rendering for tiny images (icons, small PNGs)

	m_foundFilesListCtrl
		.AddTileColumn( Dimensions )
		.AddTileColumn( Size )
		.AddTileColumn( Date )
		.AddTileColumn( Folder );
}

CAlbumSettingsDialog::~CAlbumSettingsDialog()
{
}

CMenu& CAlbumSettingsDialog::GetFileListPopupMenu( void )
{
	static CMenu s_popupMenu;
	if ( NULL == s_popupMenu.GetSafeHmenu() )
	{
		CMenu popupMenu;
		ui::LoadPopupSubMenu( s_popupMenu, IDR_CONTEXT_MENU, app::AlbumFoundListPopup );
		ui::JoinMenuItems( s_popupMenu, CPathItemListCtrl::GetStdPathListPopupMenu( CReportListControl::OnSelection ) );
	}
	return s_popupMenu;
}

bool CAlbumSettingsDialog::InitSymbolFont( void )
{
	LOGFONT logFont;
	GetFont()->GetLogFont( &logFont );

	logFont.lfHeight = MulDiv( logFont.lfHeight, 120, 100 );
	logFont.lfCharSet = SYMBOL_CHARSET;
	logFont.lfQuality = PROOF_QUALITY;
	logFont.lfPitchAndFamily = FF_DECORATIVE;
	_tcscpy( logFont.lfFaceName, _T("Wingdings") );
	return m_symbolFont.CreateFontIndirect( &logFont ) != FALSE;
}

ole::CDataSource* CAlbumSettingsDialog::NewDataSource( void )
{
	return new ole::CImagesDataSource();
}

void CAlbumSettingsDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem;
	if ( pCtrl != &m_foundFilesListCtrl )
		return;

	static const ui::CTextEffect s_errorBk( ui::Regular, app::ColorErrorText, app::ColorErrorBk );
	const CFileAttr* pFileAttr = CReportListControl::AsPtr< CFileAttr >( rowKey );

	if ( !pFileAttr->IsValid() )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( True == m_isDirty )
		rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_foundFilesListCtrl.GetTextColor(), color::White );		// blend to washed out gray effect
}

void CAlbumSettingsDialog::SetDirty( bool dirty /*= true*/ )
{
	if ( Undefined == m_isDirty )
		return;			// dialog not yet initialized, avoid altering dirty flag

	m_isDirty = dirty ? True : False;
	m_foundFilesListCtrl.Invalidate();
	m_foundFilesListCtrl.UpdateWindow();

	static const std::tstring okLabel[] = { _T("OK"), _T("&Search") };
	ui::SetDlgItemText( m_hWnd, IDOK, okLabel[ m_isDirty ] );
	ui::EnableControl( m_hWnd, IDOK, True == m_isDirty || m_fileList.GetFileOrder() != CFileList::CorruptedFiles );
}

void CAlbumSettingsDialog::UpdateFileSortOrder( void )
{
	// TODO: store display order in CFileList::m_displayOrder as std::vector< CFileAttr* >
	// - so that CFileList::m_fileAttributes keeps the original order, and is immutable
#if 1
	SetDirty( true );

	if ( m_fileList.GetFileAttrCount() < InplaceSortMaxCount )
		PostMessage( WM_COMMAND, CM_SEARCH_FOR_FILES );
#else
	switch ( m_fileList.GetFileOrder() )
	{
		case CFileList::FileSameSize:
		case CFileList::FileSameSizeAndDim:
		case CFileList::CorruptedFiles:
			SetDirty( true );
			if ( m_fileList.GetFileAttrCount() < InplaceSortMaxCount )
				PostMessage( WM_COMMAND, CM_SEARCH_FOR_FILES );
			break;
		default:
			SetupFoundListView();		// fill in the found files list
			SetDirty( false );
	}
#endif
}

std::pair< CAlbumSettingsDialog::Column, bool > CAlbumSettingsDialog::ToListSortOrder( CFileList::Order fileOrder )
{
	switch ( fileOrder )
	{
		case CFileList::ByFileNameAsc:		return std::make_pair( FileName, true );
		case CFileList::ByFileNameDesc:		return std::make_pair( FileName, false ); break;
		case CFileList::ByFullPathAsc:		return std::make_pair( Folder, true ); break;
		case CFileList::ByFullPathDesc:		return std::make_pair( Folder, false ); break;
		case CFileList::ByDimensionAsc:		return std::make_pair( Dimensions, true ); break;
		case CFileList::ByDimensionDesc:	return std::make_pair( Dimensions, false ); break;
		case CFileList::BySizeAsc:			return std::make_pair( Size, true ); break;
		case CFileList::BySizeDesc:			return std::make_pair( Size, false ); break;
		case CFileList::ByDateAsc:			return std::make_pair( Date, true ); break;
		case CFileList::ByDateDesc:			return std::make_pair( Date, false ); break;
	}
	return std::make_pair( Unordered, false );
}

// if filePath is a directory or a file path, it adds/modifies the CSearchSpec entry in m_fileList.m_searchSpecs
// if filePath points to an invalid path, it returns false.
// it finally selects the latest altered entry in m_searchSpecListBox.
bool CAlbumSettingsDialog::DropSearchSpec( const fs::CFlexPath& filePath, bool doPrompt /*= true*/ )
{
	if ( filePath.IsEmpty() || !filePath.FileExist() )
		return false;

	int index = m_fileList.FindSearchSpec( filePath );
	if ( index != -1 )
	{	// already exists -> prompt to modify
		CSearchSpec& rSearchSpec = m_fileList.m_searchSpecs[ index ];

		m_searchSpecListBox.SetCurSel( index );

		if ( doPrompt )
		{
			CSearchSpecDialog dlg( rSearchSpec, this );
			if ( IDCANCEL == dlg.DoModal() )
				return false;
			rSearchSpec = dlg.m_searchSpec;
		}
		m_searchSpecListBox.DeleteString( index );
		m_searchSpecListBox.InsertString( index, rSearchSpec.m_searchPath.GetPtr() );
	}
	else
	{
		CSearchSpec searchSpec( filePath );

		m_fileList.m_searchSpecs.push_back( searchSpec );
		index = (int)m_fileList.m_searchSpecs.size() - 1;
		m_searchSpecListBox.AddString( searchSpec.m_searchPath.GetPtr() );
	}
	m_searchSpecListBox.SetCurSel( index );

	SetDirty();
	OnLBnSelChange_SearchSpec();
	return true;
}

bool CAlbumSettingsDialog::MoveSearchSpec( int moveBy )
{
	int selIndex = m_searchSpecListBox.GetCurSel(), newIndex = selIndex + moveBy;

	ASSERT( selIndex != LB_ERR && moveBy != 0 );
	m_searchSpecListBox.DeleteString( selIndex );

	CSearchSpec searchSpec = m_fileList.m_searchSpecs[ selIndex ];

	m_fileList.m_searchSpecs.erase( m_fileList.m_searchSpecs.begin() + selIndex );
	m_fileList.m_searchSpecs.insert( m_fileList.m_searchSpecs.begin() + newIndex, searchSpec );

	if ( newIndex < m_searchSpecListBox.GetCount() )
		m_searchSpecListBox.InsertString( newIndex, searchSpec.m_searchPath.GetPtr() );
	else
		m_searchSpecListBox.AddString( searchSpec.m_searchPath.GetPtr() );

	VERIFY( m_searchSpecListBox.SetCurSel( newIndex ) != LB_ERR );
	SetDirty();
	OnLBnSelChange_SearchSpec();
	return true;
}

bool CAlbumSettingsDialog::AddSearchSpec( int index )
{
	ASSERT( index != LB_ERR );

	CSearchSpecDialog dlg( CSearchSpec(), this );
	if ( dlg.DoModal() != IDOK )
		return false;

	int dupIndex = CheckForDuplicates( dlg.m_searchSpec.m_searchPath.GetPtr() );
	if ( dupIndex != -1 )
	{	// modify the same entry instead of
		index = dupIndex;
		m_fileList.m_searchSpecs.erase( m_fileList.m_searchSpecs.begin() + index );
		m_searchSpecListBox.DeleteString( index );
	}
	m_fileList.m_searchSpecs.insert( m_fileList.m_searchSpecs.begin() + index, dlg.m_searchSpec );
	m_searchSpecListBox.InsertString( index, m_fileList.m_searchSpecs[ index ].m_searchPath.GetPtr() );

	m_searchSpecListBox.SetCurSel( index );
	SetDirty();
	OnLBnSelChange_SearchSpec();
	return true;
}

bool CAlbumSettingsDialog::ModifySearchSpec( int index )
{
	ASSERT( index != LB_ERR );

	CSearchSpecDialog dlg( m_fileList.m_searchSpecs[ index ], this );
	if ( dlg.DoModal() != IDOK )
		return false;

	int dupIndex = CheckForDuplicates( dlg.m_searchSpec.m_searchPath.GetPtr(), index );
	if ( dupIndex != -1 )
	{	// modify the same entry
		m_fileList.m_searchSpecs.erase( m_fileList.m_searchSpecs.begin() + index );
		m_searchSpecListBox.DeleteString( index );
		index = dupIndex;
	}

	m_fileList.m_searchSpecs[ index ] = dlg.m_searchSpec;
	m_searchSpecListBox.DeleteString( index );
	m_searchSpecListBox.InsertString( index, m_fileList.m_searchSpecs[ index ].m_searchPath.GetPtr() );

	m_searchSpecListBox.SetCurSel( index );
	SetDirty();
	OnLBnSelChange_SearchSpec();
	return true;
}

bool CAlbumSettingsDialog::DeleteSearchSpec( int index, bool doPrompt /*= false*/ )
{
	ASSERT( index != LB_ERR );
	if ( doPrompt )
		if ( IDCANCEL == AfxMessageBox( str::Format( IDS_DELETE_ATTR_PROMPT, m_fileList.m_searchSpecs[ index ].m_searchPath.GetPtr() ).c_str(), MB_OKCANCEL ) )
			return false;

	m_fileList.m_searchSpecs.erase( m_fileList.m_searchSpecs.begin() + index );
	m_searchSpecListBox.DeleteString( index );
	index = std::min( index, m_searchSpecListBox.GetCount() - 1 );
	m_searchSpecListBox.SetCurSel( index );

	SetDirty();
	OnLBnSelChange_SearchSpec();
	return true;
}


namespace hlp
{
	std::tstring GetItemText( const CListBox& rListBox, int pos )
	{
		CString itemText;
		rListBox.GetText( pos, itemText );
		return itemText.GetString();
	}

	int FindPathIndex( const CListBox& rListBox, const std::tstring& match, int ignoreIndex = -1 )
	{
		for ( int i = 0, count = rListBox.GetCount(); i < count; ++i )
			if ( i != ignoreIndex )
			{
				std::tstring itemText = GetItemText( rListBox, i );
				if ( path::Equivalent( itemText, match ) )
					return i;
			}

		return -1;
	}
}

int CAlbumSettingsDialog::CheckForDuplicates( const TCHAR* pFilePath, int ignoreIndex /*= -1*/ )
{
	int foundIndex = hlp::FindPathIndex( m_searchSpecListBox, pFilePath, ignoreIndex );
	if ( foundIndex != -1 )
		if ( AfxMessageBox( str::Format( IDS_REPLACE_DUP_PROMPT, pFilePath ).c_str(), MB_YESNO | MB_ICONQUESTION ) != IDYES )
			return -1;		// allow the duplicate to exist

	return foundIndex;
}

bool CAlbumSettingsDialog::SearchSourceFiles( void )
{
	CWaitCursor wait;
	bool success = true;

	// Note:
	// Freeze list redraw since file search progress notifications force listCtrl redraw while old image items in m_fileList have been deleted, leaving dangling references in the list.
	CScopedLockRedraw freeze( &m_foundFilesListCtrl );			// to prevent list redraw crash due to dangling reference image items

	try
	{
		m_fileList.SearchForFiles();
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		success = false;
	}

	SetupFoundListView();		// fill in the found files list
	SetDirty( false );
	return success;
}

void CAlbumSettingsDialog::SetupFoundListView( void )
{
	CScopedListTextSelection scopedSel( &m_foundFilesListCtrl );

	size_t count = m_fileList.GetFileAttrCount();
	{
		CScopedInternalChange internalChange( &m_foundFilesListCtrl );
		CScopedLockRedraw freeze( &m_foundFilesListCtrl );

		// fill in the found files list (File Name|In Folder|Size|Date)
		m_foundFilesListCtrl.DeleteAllItems();

		for ( UINT i = 0; i != count; ++i )
		{
			const CFileAttr* pFileAttr = &m_fileList.GetFileAttr( i );

			m_foundFilesListCtrl.InsertObjectItem( i, const_cast< CFileAttr* >( pFileAttr ), ui::Transparent_Image );
			m_foundFilesListCtrl.SetSubItemText( i, Folder, pFileAttr->GetPath().GetOriginParentPath().Get() );
			m_foundFilesListCtrl.SetItemText( i, Dimensions, LPSTR_TEXTCALLBACK );			// defer CPU-intensive dimensions evaluation

			std::tstring sizeText;
			switch ( m_fileList.GetFileOrder() )
			{
				case CFileList::FileSameSize:		sizeText = pFileAttr->FormatFileSize( 1, _T("%s B") ); break;
				case CFileList::FileSameSizeAndDim:	sizeText = pFileAttr->FormatFileSize( 1, _T("%s B (%dx%d)") ); break;
				default:							sizeText = pFileAttr->FormatFileSize();
			}

			m_foundFilesListCtrl.SetSubItemText( i, Size, sizeText );
			m_foundFilesListCtrl.SetSubItemText( i, Date, pFileAttr->FormatLastModifTime() );
		}
	}

	ui::SetDlgItemText( m_hWnd, IDC_FOUND_FILES_STATIC, str::Format( _T("&Found %d file(s):"), count ) );
}

void CAlbumSettingsDialog::QueryFoundListSelection( std::vector< std::tstring >& rSelFilePaths, bool clearInvalidFiles /*= true*/ )
{
	std::vector< CFileAttr* > selFileAttrs;
	m_foundFilesListCtrl.QuerySelectionAs( selFileAttrs );

	rSelFilePaths.clear();
	rSelFilePaths.reserve( selFileAttrs.size() );

	bool foundInvalid = false;

	for ( std::vector< CFileAttr* >::const_iterator itSelFileAttr = selFileAttrs.begin(); itSelFileAttr != selFileAttrs.end(); ++itSelFileAttr )
		if ( ( *itSelFileAttr )->IsValid() )
			rSelFilePaths.push_back( ( *itSelFileAttr )->GetPath().Get() );
		else if ( clearInvalidFiles )
		{
			foundInvalid = true;
			m_foundFilesListCtrl.SetSelected( m_foundFilesListCtrl.FindItemIndex( *itSelFileAttr ), false );		// un-select invalid image path
		}

	// clear selection for non-existing files (if any)
	if ( foundInvalid )
	{
		ui::BeepSignal();
		m_foundFilesListCtrl.Invalidate();
	}
}

int CAlbumSettingsDialog::GetCheckStateAutoRegen( void ) const
{
	if ( m_fileList.IsAutoDropRecipient() )
		return BST_INDETERMINATE;

	return HasFlag( m_fileList.m_perFlags, CFileList::AutoRegenerate ) ? BST_CHECKED : BST_UNCHECKED;
}

void CAlbumSettingsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_foundFilesListCtrl.m_hWnd;
	CFileList::Order fileOrder = m_fileList.GetFileOrder();

	DDX_Control( pDX, IDC_MIN_FILE_SIZE_EDIT, m_minSizeEdit );
	DDX_Control( pDX, IDC_MAX_FILE_SIZE_EDIT, m_maxSizeEdit );
	DDX_Control( pDX, CM_MOVE_DOWN_SEARCH_SPEC, m_moveDownButton );
	DDX_Control( pDX, CM_MOVE_UP_SEARCH_SPEC, m_moveUpButton );
	DDX_Control( pDX, IDC_SEARCH_SPEC_LIST, m_searchSpecListBox );
	ui::DDX_EnumCombo( pDX, IDC_LIST_ORDER_COMBO, m_sortOrderCombo, fileOrder, CFileList::GetTags_Order() );
	DDX_Control( pDX, IDC_FOUND_FILES_LISTVIEW, m_foundFilesListCtrl );
	DDX_Control( pDX, IDC_THUMB_PREVIEW_STATIC, m_thumbPreviewCtrl );
	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignRight | V_AlignCenter );

	if ( firstInit )
		m_foundFilesListCtrl.SetCompactIconSpacing();

	switch ( pDX->m_bSaveAndValidate )
	{
		case DialogOutput:
		{	// dialog setup
			CheckDlgButton( IDC_MIN_FILE_SIZE_CHECK, m_fileList.m_fileSizeRange.m_start != 0 );
			CheckDlgButton( IDC_MAX_FILE_SIZE_CHECK, m_fileList.m_fileSizeRange.m_end != UINT_MAX );
			OnToggle_MinSize();
			OnToggle_MaxSize();

			// auto-drop side effect: also auto regenerate, but disable the check
			CheckDlgButton( IDC_AUTO_REGENERATE_CHECK, GetCheckStateAutoRegen() );
			CheckDlgButton( IDC_AUTO_DROP_CHECK, m_fileList.IsAutoDropRecipient() );

			// fill in the search attribute path list
			m_searchSpecListBox.ResetContent();
			for ( size_t i = 0; i != m_fileList.m_searchSpecs.size(); ++i )
				m_searchSpecListBox.AddString( m_fileList.m_searchSpecs[ i ].m_searchPath.GetPtr() );
			m_searchSpecListBox.SetCurSel( 0 );
			OnLBnSelChange_SearchSpec();

			// setup list-control
			std::pair< Column, bool > sortPair = ToListSortOrder( fileOrder );
			m_foundFilesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

			SetupFoundListView();		// fill in the found files list
			if ( -1 == m_foundFilesListCtrl.GetCaretIndex() )
				if ( (UINT)m_currentIndex < m_fileList.GetFileAttrCount() )
					m_foundFilesListCtrl.SetCurSel( m_currentIndex );
				else if ( m_fileList.AnyFoundFiles() )
					m_foundFilesListCtrl.SetCurSel( 0 );

			int selIndex = m_foundFilesListCtrl.GetCurSel();

			if ( selIndex != -1 )
				m_thumbPreviewCtrl.SetImagePath( m_fileList.GetFileAttr( selIndex ).GetPath() );
			else
				m_thumbPreviewCtrl.SetImagePath( fs::CFlexPath() );

			// set initial dirty if auto-generation is turned on (either explicit or by auto-drop)
			SetDirty( m_fileList.MustAutoRegenerate() );
			break;
		}
		case DialogSaveChanges:
		{
			m_fileList.SetFileOrder( fileOrder );

			size_t number = 0;
			bool validNumber;

			validNumber = IsDlgButtonChecked( IDC_MIN_FILE_SIZE_CHECK ) != FALSE;
			if ( validNumber )
				number = m_minSizeEdit.GetNumber< size_t >( &validNumber ) * KiloByte;
			m_fileList.m_fileSizeRange.m_start = validNumber ? number : 0;

			validNumber = IsDlgButtonChecked( IDC_MAX_FILE_SIZE_CHECK ) != FALSE;
			if ( validNumber )
				number = m_maxSizeEdit.GetNumber< size_t >( &validNumber ) * KiloByte;
			m_fileList.m_fileSizeRange.m_end = validNumber ? number : UINT_MAX;

			UINT ckState = IsDlgButtonChecked( IDC_AUTO_REGENERATE_CHECK );
			if ( ckState != BST_INDETERMINATE )
				SetFlag( m_fileList.m_perFlags, CFileList::AutoRegenerate, BST_CHECKED == ckState );		// the button is checked naturally, and not as a side effect of auto-drop feature

			m_currentIndex = m_foundFilesListCtrl.GetCaretIndex();
			if ( -1 == m_currentIndex )
				m_currentIndex = 0;
			break;
		}
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumSettingsDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
	ON_CBN_SELCHANGE( IDC_LIST_ORDER_COMBO, OnCBnSelChange_SortOrder )
	ON_BN_CLICKED( IDC_MIN_FILE_SIZE_CHECK, OnToggle_MinSize )
	ON_BN_CLICKED( IDC_MAX_FILE_SIZE_CHECK, OnToggle_MaxSize )
	ON_EN_CHANGE( IDC_MIN_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_EN_CHANGE( IDC_MAX_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_BN_CLICKED( IDC_AUTO_REGENERATE_CHECK, OnToggle_AutoRegenerate )
	ON_BN_CLICKED( IDC_AUTO_DROP_CHECK, OnToggle_AutoDrop )
	ON_LBN_SELCHANGE( IDC_SEARCH_SPEC_LIST, OnLBnSelChange_SearchSpec )
	ON_LBN_DBLCLK( IDC_SEARCH_SPEC_LIST, OnLBnDblclk_SearchSpec )
	ON_BN_CLICKED( CM_MOVE_UP_SEARCH_SPEC, OnMoveUp_SearchSpec )
	ON_BN_CLICKED( CM_MOVE_DOWN_SEARCH_SPEC, OnMoveDown_SearchSpec )
	ON_BN_CLICKED( CM_ADD_SEARCH_SPEC, OnAdd_SearchSpec )
	ON_BN_CLICKED( CM_MODIFY_SEARCH_SPEC, OnModify_SearchSpec )
	ON_BN_CLICKED( CM_DELETE_SEARCH_SPEC, OnDelete_SearchSpec )
	ON_BN_CLICKED( CM_SEARCH_FOR_FILES, OnSearchSourceFiles )
	ON_COMMAND_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, On_OrderRandomShuffle )
	ON_UPDATE_COMMAND_UI_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, OnUpdate_OrderRandomShuffle )
	ON_NOTIFY( LVN_COLUMNCLICK, IDC_FOUND_FILES_LISTVIEW, OnLVnColumnClick_FoundFiles )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FOUND_FILES_LISTVIEW, OnLVnItemChanged_FoundFiles )
	ON_NOTIFY( LVN_GETDISPINFO, IDC_FOUND_FILES_LISTVIEW, OnLVnGetDispInfo_FoundFiles )
	ON_CONTROL( lv::LVN_ItemsReorder, IDC_FOUND_FILES_LISTVIEW, OnLVnItemsReorder_FoundFiles )
	ON_STN_CLICKED( IDC_THUMB_PREVIEW_STATIC, &CAlbumSettingsDialog::OnStnClickedThumbPreviewStatic )
	// image file operations: CM_OPEN_IMAGE_FILE, CM_DELETE_FILE, CM_DELETE_FILE_NO_UNDO, CM_MOVE_FILE, CM_EXPLORE_IMAGE
	ON_COMMAND_RANGE( CM_OPEN_IMAGE_FILE, CM_EXPLORE_IMAGE, OnImageFileOp )
END_MESSAGE_MAP()

BOOL CAlbumSettingsDialog::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( m_searchListAccel.TranslateIfOwnsFocus( pMsg, m_hWnd, m_searchSpecListBox ) )
			return true;
		else if ( m_dlgAccel.Translate( pMsg, m_hWnd ) )
			return true;

	return CLayoutDialog::PreTranslateMessage( pMsg );
}

BOOL CAlbumSettingsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	switch ( id )
	{
		case ID_EDIT_COPY:
			// special case: handled by the file list (since CDialog::OnCmdMsg() routes to the parent frame window)
			return m_foundFilesListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
	}

	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_foundFilesListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BOOL CAlbumSettingsDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();
	DragAcceptFiles();
	InitSymbolFont();

	m_moveUpButton.SetFont( &m_symbolFont );
	m_moveDownButton.SetFont( &m_symbolFont );

	m_isDirty = False;
	return TRUE;
}

void CAlbumSettingsDialog::OnDestroy( void )
{
	CLayoutDialog::OnDestroy();
}

void CAlbumSettingsDialog::OnOK( void )
{
	if ( !UpdateData( DialogSaveChanges ) )
		TRACE( _T("UpdateData failed during dialog termination.\n") );		// UpdateData() will set focus to correct item
	else if ( True == m_isDirty )
		SearchSourceFiles();
	else
		CLayoutDialog::OnOK();
}

void CAlbumSettingsDialog::OnCancel( void )
{
	if ( !m_fileList.CancelGeneration() )
		CLayoutDialog::OnCancel();
}

void CAlbumSettingsDialog::OnDropFiles( HDROP hDropInfo )
{
	SetActiveWindow();

	UINT fileCount = ::DragQueryFile( hDropInfo, ( UINT )-1, NULL, 0 );

	for ( UINT i = 0; i != fileCount; ++i )
	{
		TCHAR filePath[ MAX_PATH ];
		::DragQueryFile( hDropInfo, i, filePath, COUNT_OF( filePath ) );
		DropSearchSpec( fs::CFlexPath( filePath ) );
	}
	::DragFinish( hDropInfo );
}

void CAlbumSettingsDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
	if ( pWnd == &m_foundFilesListCtrl )
	{
		static CMenu contextMenu;
		if ( NULL == (HMENU)contextMenu )
			ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AlbumFoundListPopup );

		if ( m_foundFilesListCtrl.GetSelectedCount() != 0 )
			if ( (HMENU)contextMenu != NULL )
				contextMenu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
	}
}

void CAlbumSettingsDialog::OnCBnSelChange_SortOrder( void )
{
	CFileList::Order fileOrder = static_cast< CFileList::Order >( m_sortOrderCombo.GetCurSel() );
	m_fileList.SetFileOrder( fileOrder );

	std::pair< Column, bool > sortPair = ToListSortOrder( fileOrder );
	m_foundFilesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

	UpdateFileSortOrder();
}

void CAlbumSettingsDialog::OnToggle_MinSize( void )
{
	if ( IsDlgButtonChecked( IDC_MIN_FILE_SIZE_CHECK ) )
	{
		ui::EnableWindow( m_minSizeEdit );
		m_minSizeEdit.SetNumber( m_fileList.m_fileSizeRange.m_start / KiloByte );
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
		ui::EnableWindow( m_maxSizeEdit );
		m_maxSizeEdit.SetNumber( m_fileList.m_fileSizeRange.m_end / KiloByte );
	}
	else
	{
		ui::EnableWindow( m_maxSizeEdit, false );
		ui::SetWindowText( m_maxSizeEdit, std::tstring() );
	}
	SetDirty();
}

void CAlbumSettingsDialog::OnEnChange_MinMaxSize( void )
{
	SetDirty();
}

void CAlbumSettingsDialog::OnLBnSelChange_SearchSpec( void )
{
	int selIndex = m_searchSpecListBox.GetCurSel();
	int attrCount = (int)m_fileList.m_searchSpecs.size();

	ASSERT( selIndex != LB_ERR || 0 == m_searchSpecListBox.GetCount() );
	ui::EnableWindow( m_moveUpButton, selIndex > 0 );
	ui::EnableWindow( m_moveDownButton, selIndex < attrCount - 1 );

	ui::EnableControl( m_hWnd, CM_MODIFY_SEARCH_SPEC, selIndex != LB_ERR );
	ui::EnableControl( m_hWnd, CM_DELETE_SEARCH_SPEC, selIndex != LB_ERR );

	CheckDlgButton( IDC_AUTO_REGENERATE_CHECK, GetCheckStateAutoRegen() );
	CheckDlgButton( IDC_AUTO_DROP_CHECK, m_fileList.IsAutoDropRecipient() );
}

void CAlbumSettingsDialog::OnLBnDblclk_SearchSpec( void )
{
	if ( m_searchSpecListBox.GetCount() > 0 )
		OnModify_SearchSpec();
}

void CAlbumSettingsDialog::OnMoveUp_SearchSpec( void )
{
	MoveSearchSpec( -1 );
}

void CAlbumSettingsDialog::OnMoveDown_SearchSpec( void )
{
	MoveSearchSpec( 1 );
}

void CAlbumSettingsDialog::OnAdd_SearchSpec( void )
{
	int attrCount = (int)m_fileList.GetSearchSpecCount();
	int atIndex = m_searchSpecListBox.GetCurSel();

	if ( LB_ERR == atIndex )
		if ( 0 == attrCount )
			atIndex = 0;
		else
			ASSERT( false );

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		atIndex = attrCount;

	AddSearchSpec( atIndex );
}

void CAlbumSettingsDialog::OnModify_SearchSpec( void )
{
	ModifySearchSpec( m_searchSpecListBox.GetCurSel() );
}

void CAlbumSettingsDialog::OnDelete_SearchSpec( void )
{
	bool doPrompt = !ui::IsKeyPressed( VK_SHIFT );
	DeleteSearchSpec( m_searchSpecListBox.GetCurSel(), doPrompt );
}

void CAlbumSettingsDialog::OnSearchSourceFiles( void )
{
	SearchSourceFiles();
}

void CAlbumSettingsDialog::On_OrderRandomShuffle( UINT cmdId )
{
	CFileList::Order fileOrder = hlp::OrderOfCmd( cmdId );

	m_fileList.SetFileOrder( fileOrder );
	m_sortOrderCombo.SetCurSel( fileOrder );
	UpdateFileSortOrder();
}

void CAlbumSettingsDialog::OnUpdate_OrderRandomShuffle( CCmdUI* pCmdUI )
{
	CFileList::Order fileOrder = hlp::OrderOfCmd( pCmdUI->m_nID );

	pCmdUI->Enable( !m_fileList.InGeneration() );
	pCmdUI->SetCheck( fileOrder == m_fileList.GetFileOrder() );
}

void CAlbumSettingsDialog::OnImageFileOp( UINT cmdId )
{
	std::vector< std::tstring > targetFiles;
	QueryFoundListSelection( targetFiles );
	if ( targetFiles.empty() )
		return;

	switch ( cmdId )
	{
		case CM_OPEN_IMAGE_FILE:
			for ( std::vector< std::tstring >::const_iterator it = targetFiles.begin(); it != targetFiles.end(); ++it )
				app::GetApp()->OpenDocumentFile( it->c_str() );
			break;
		case CM_DELETE_FILE:
		case CM_DELETE_FILE_NO_UNDO:
			app::DeleteFiles( targetFiles, CM_DELETE_FILE == cmdId );
			break;
		case CM_MOVE_FILE:
			app::MoveFiles( targetFiles, this );
			break;
		case CM_EXPLORE_IMAGE:
			for ( std::vector< std::tstring >::const_iterator it = targetFiles.begin(); it != targetFiles.end(); ++it )
				shell::ExploreAndSelectFile( it->c_str() );
			break;
		default:
			ASSERT( false );
	}
}

void CAlbumSettingsDialog::OnToggle_AutoRegenerate( void )
{
	UINT ckState = IsDlgButtonChecked( IDC_AUTO_REGENERATE_CHECK );
	if ( ckState != BST_INDETERMINATE )
		SetFlag( m_fileList.m_perFlags, CFileList::AutoRegenerate, BST_CHECKED == ckState );		// the button is checked naturally, and not as a side effect of auto-drop feature
	SetDirty();
}

void CAlbumSettingsDialog::OnToggle_AutoDrop( void )
{
	AfxMessageBox( IDS_NO_DIR_SEARCH_SPEC );
}

void CAlbumSettingsDialog::OnLVnColumnClick_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr;
	CFileList::Order fileOrder = m_fileList.GetFileOrder();

	switch ( pNmListView->iSubItem )
	{
		case FileName:
			fileOrder = fileOrder == CFileList::ByFileNameAsc ? CFileList::ByFileNameDesc : CFileList::ByFileNameAsc;
			break;
		case Folder:
			fileOrder = fileOrder == CFileList::ByFullPathAsc ? CFileList::ByFullPathDesc : CFileList::ByFullPathAsc;
			break;
		case Dimensions:
			fileOrder = fileOrder == CFileList::ByDimensionAsc ? CFileList::ByDimensionDesc : CFileList::ByDimensionAsc;
			break;
		case Size:
			fileOrder = fileOrder == CFileList::BySizeAsc ? CFileList::BySizeDesc : CFileList::BySizeAsc;
			break;
		case Date:
			fileOrder = fileOrder == CFileList::ByDateAsc ? CFileList::ByDateDesc : CFileList::ByDateAsc;
			break;
	}
	m_fileList.SetFileOrder( fileOrder );
	m_sortOrderCombo.SetCurSel( fileOrder );
	UpdateFileSortOrder();
	*pResult = 0;
}

void CAlbumSettingsDialog::OnLVnItemChanged_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr;

	if ( pNmListView->iItem != -1 )
		if ( pNmListView->uChanged & LVIF_STATE )
			if ( ( pNmListView->uNewState & ( LVIS_FOCUSED | LVIS_SELECTED ) ) != ( pNmListView->uOldState & ( LVIS_FOCUSED | LVIS_SELECTED ) ) )
				if ( pNmListView->uNewState & LVIS_FOCUSED )
					m_thumbPreviewCtrl.SetImagePath( m_fileList.GetFileAttr( pNmListView->iItem ).GetPath() );

	*pResult = 0;
}

void CAlbumSettingsDialog::OnLVnGetDispInfo_FoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDisplayInfo = (LV_DISPINFO*)pNmHdr;

	if ( HasFlag( pDisplayInfo->item.mask, LVIF_TEXT ) )
		switch ( pDisplayInfo->item.iSubItem )
		{
			case Dimensions:
			{
				const CFileAttr* pFileAttr = CReportListControl::AsPtr< CFileAttr >( pDisplayInfo->item.lParam );
				const CSize& dim = pFileAttr->GetImageDim();
				std::tstring text = str::Format( _T("%dx%d"), dim.cx, dim.cy );
				_tcsncpy( pDisplayInfo->item.pszText, text.c_str(), pDisplayInfo->item.cchTextMax );
				break;
			}
		}

	*pResult = 0;
}

void CAlbumSettingsDialog::OnLVnItemsReorder_FoundFiles( void )
{
	// input custom order
	std::vector< CFileAttr* > customOrder;
	customOrder.reserve( m_fileList.GetFileAttrCount() );

	for ( int i = 0, count = m_foundFilesListCtrl.GetItemCount(); i != count; ++i )
		customOrder.push_back( m_foundFilesListCtrl.GetPtrAt< CFileAttr >( i ) );

	m_fileList.SetCustomOrder( customOrder );

	// update UI
	CFileList::Order fileOrder = m_fileList.GetFileOrder();
	m_sortOrderCombo.SetCurSel( fileOrder );
	std::pair< Column, bool > sortPair = ToListSortOrder( fileOrder );
	m_foundFilesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );
}

void CAlbumSettingsDialog::OnStnClickedThumbPreviewStatic( void )
{
	shell::Execute( this, m_thumbPreviewCtrl.GetImagePath().GetPtr() );
}
