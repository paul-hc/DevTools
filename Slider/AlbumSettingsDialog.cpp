
#include "stdafx.h"
#include "AlbumSettingsDialog.h"
#include "FileAttrAlgorithms.h"
#include "MainFrame.h"
#include "ImageView.h"
#include "SearchPatternDialog.h"
#include "OleImagesDataSource.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/RuntimeException.h"
#include "utl/UI/Color.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"
#include "utl/UI/DragListCtrl.hxx"
#include "utl/Resequence.hxx"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


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
		{ IDC_GROUP_BOX_1, SizeX | DoRepaint },
		{ IDC_PATTERNS_LISTVIEW, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

			{ IDC_SEARCH_SPEC_LIST, SizeX },
			{ IDC_SEARCH_SPEC_MOVE_UP, MoveX },
			{ IDC_SEARCH_SPEC_MOVE_DOWN, MoveX },

		{ IDC_THUMB_PREVIEW_STATIC, MoveX },
		{ IDC_DOC_VERSION_LABEL, MoveX },
		{ IDC_DOC_VERSION_STATIC, MoveX },

		{ IDC_FOUND_IMAGES_LISTVIEW, Size },
		{ IDC_STRIP_BAR_2, MoveX },
		{ IDC_LIST_ORDER_STATIC, MoveX },
		{ IDC_LIST_ORDER_COMBO, MoveX },

		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


const ui::CTextEffect CAlbumSettingsDialog::s_newFileEffect( ui::Regular, color::Blue );

CAlbumSettingsDialog::CAlbumSettingsDialog( const CAlbumModel& model, size_t currentPos, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_ALBUM_SETTINGS_DIALOG, pParent )
	, m_model( model )
	, m_pCaretFileAttr( currentPos < m_model.GetFileAttrCount() ? m_model.GetFileAttr( currentPos ) : NULL )
	, m_isDirty( false )

	, m_patternsListCtrl( IDC_PATTERNS_LISTVIEW )
	, m_patternsEditor( &m_patternsListCtrl )
	, m_maxFileCountEdit( true, str::GetUserLocale() )
	, m_minSizeEdit( true, str::GetUserLocale() )
	, m_maxSizeEdit( true, str::GetUserLocale() )
	, m_sortOrderCombo( &fattr::GetTags_Order() )
	, m_thumbPreviewCtrl( app::GetThumbnailer() )
	, m_docVersionLabel( CRegularStatic::ControlLabel )
	, m_docVersionStatic( CRegularStatic::Bold )
	, m_imagesListCtrl( IDC_FOUND_IMAGES_LISTVIEW )
{
	// base init
	m_regSection = _T("AlbumSettingsDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	m_initCentered = false;
	LoadDlgIcon( ID_EDIT_ALBUM );
	m_accelPool.AddAccelTable( new CAccelTable( IDD_ALBUM_SETTINGS_DIALOG ) );

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
		.AddButton( IDC_SEARCH_FOR_FILES, CM_EXPLORE_IMAGE );

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

	m_patternsListCtrl.SetSection( m_regSection + _T("\\PatternsList") );
	m_imagesListCtrl.SetCustomImageDraw( app::GetThumbnailer() );

	m_imagesListCtrl.SetSection( m_regSection + _T("\\ImagesList") );
	m_imagesListCtrl.SetCustomImageDraw( app::GetThumbnailer() );
	m_imagesListCtrl.SetTextEffectCallback( this );
	m_imagesListCtrl.SetSortInternally( false );
	m_imagesListCtrl.SetUseAlternateRowColoring();
	m_imagesListCtrl.SetDataSourceFactory( this );						// uses temporary file clones for embedded images
	m_imagesListCtrl.SetTrackMenuTarget( this );						// let dialog track SPECIFIC custom menu commands (Explorer verbs handled by the listctrl)
	m_imagesListCtrl.SetPopupMenu( CReportListControl::OnSelection, &GetAlbumModelPopupMenu() );

	ClearFlag( m_imagesListCtrl.RefListStyleEx(), LVS_EX_DOUBLEBUFFER );		// better looking thumb rendering for tiny images (icons, small PNGs)

	m_imagesListCtrl
		.AddTileColumn( Dimensions )
		.AddTileColumn( Size )
		.AddTileColumn( Date )
		.AddTileColumn( Folder );
}

CAlbumSettingsDialog::~CAlbumSettingsDialog()
{
}

int CAlbumSettingsDialog::GetCurrentIndex( void ) const
{
	if ( NULL == m_pCaretFileAttr )
		return m_model.AnyFoundFiles() ? 0 : -1;

	return static_cast<int>( utl::FindPos( m_model.GetImagesModel().GetFileAttrs(), m_pCaretFileAttr ) );
}

CMenu& CAlbumSettingsDialog::GetAlbumModelPopupMenu( void )
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

void CAlbumSettingsDialog::SetupPatternsListView( void )
{
	CScopedInternalChange internalChange( &m_patternsListCtrl );
	CScopedLockRedraw freeze( &m_patternsListCtrl );

	// fill in the found files list (File Name|In Folder|Size|Date)
	m_patternsListCtrl.DeleteAllItems();

	const std::vector< CSearchPattern* >& patterns = m_model.GetSearchModel()->GetPatterns();

	for ( UINT i = 0, count = static_cast<UINT>( patterns.size() ); i != count; ++i )
	{
		CSearchPattern* pPattern = patterns[ i ];

		m_patternsListCtrl.InsertObjectItem( i, pPattern, ui::Transparent_Image );
		m_patternsListCtrl.SetSubItemText( i, PatternType, CSearchPattern::GetTags_Type().FormatUi( pPattern->GetType() ) );
		m_patternsListCtrl.SetSubItemText( i, PatternDepth, CSearchPattern::GetTags_SearchMode().FormatUi( pPattern->GetSearchMode() ) );

//		if ( utl::Contains( m_newFilePaths, pPattern->GetPath() ) )
//			m_patternsListCtrl.MarkRowAt( i, s_newFileEffect );
	}
}

void CAlbumSettingsDialog::SetupFoundImagesListView( void )
{
	CWaitCursor wait;
	CScopedListTextSelection scopedSel( &m_imagesListCtrl );

	size_t count = m_model.GetFileAttrCount();
	{
		CScopedInternalChange internalChange( &m_imagesListCtrl );
		CScopedLockRedraw freeze( &m_imagesListCtrl );

		// fill in the found files list (File Name|In Folder|Size|Date)
		m_imagesListCtrl.DeleteAllItems();

		for ( UINT i = 0; i != count; ++i )
		{
			const CFileAttr* pFileAttr = m_model.GetFileAttr( i );

			m_imagesListCtrl.InsertObjectItem( i, const_cast< CFileAttr* >( pFileAttr ), ui::Transparent_Image );
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

			if ( utl::Contains( m_newFilePaths, pFileAttr->GetPath() ) )
				m_imagesListCtrl.MarkRowAt( i, s_newFileEffect );
		}
	}

	ui::SetDlgItemText( m_hWnd, IDC_FOUND_FILES_STATIC, str::Format( _T("&Found %d file(s):"), count ) );
}

void CAlbumSettingsDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem;
	if ( pCtrl != &m_imagesListCtrl )
		return;

	static const ui::CTextEffect s_errorBk( ui::Regular, app::ColorErrorText, app::ColorErrorBk );
	const CFileAttr* pFileAttr = CReportListControl::AsPtr< CFileAttr >( rowKey );

	if ( !pFileAttr->IsValid() )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( m_isDirty )
	{
		if ( CLR_NONE == rTextEffect.m_textColor )
			rTextEffect.m_textColor = m_imagesListCtrl.GetTextColor();

		if ( CLR_DEFAULT == rTextEffect.m_textColor )
			rTextEffect.m_textColor = ::GetSysColor( COLOR_WINDOWTEXT );

		rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor, color::White );		// blend to washed out gray effect
	}
}

bool CAlbumSettingsDialog::SetDirty( bool dirty /*= true*/ )
{
	if ( IsInternalChange() )
		return false;			// during dialog output

	m_isDirty = dirty;
	m_imagesListCtrl.Invalidate();
	m_imagesListCtrl.UpdateWindow();

	static const struct { std::tstring m_caption; UINT m_iconId; } s_okLabelIcon[] = { { _T("OK"), 0 }, { _T("&Search"), CM_EXPLORE_IMAGE } };
	m_okButton.SetButtonCaption( s_okLabelIcon[ m_isDirty ].m_caption );
	m_okButton.SetIconId( s_okLabelIcon[ m_isDirty ].m_iconId );

	ui::EnableControl( m_hWnd, IDOK, m_isDirty || m_model.GetFileOrder() != fattr::FilterCorruptedFiles );
	return true;
}

std::pair< CAlbumSettingsDialog::ImagesColumn, bool > CAlbumSettingsDialog::ToListSortOrder( fattr::Order fileOrder )
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


// if filePath is a directory or a file path, it adds/modifies the CSearchPattern entry in m_model.m_searchPatterns
// if filePath points to an invalid path, it returns false.
// it finally selects the latest altered entry in the list
bool CAlbumSettingsDialog::DropSearchPattern( const fs::CFlexPath& filePath, bool doPrompt /*= true*/ )
{
	if ( filePath.IsEmpty() || !filePath.FileExist() )
		return false;

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	int index = pSearchModel->FindPatternPos( filePath );

	if ( index != -1 )
	{	// already exists -> prompt to modify
		CSearchPattern* pSearchPattern = pSearchModel->GetPatternAt( index );

		m_searchPatternListBox.SetCurSel( index );

		if ( doPrompt )
		{
			CSearchPatternDialog dlg( *pSearchPattern, this );
			if ( IDCANCEL == dlg.DoModal() )
				return false;

			*pSearchPattern = dlg.m_searchPattern;
		}
		m_searchPatternListBox.DeleteString( index );
		m_searchPatternListBox.InsertString( index, pSearchPattern->GetFilePath().GetPtr() );
	}
	else
	{
		pSearchModel->AddSearchPath( filePath );
		index = static_cast<int>( pSearchModel->GetPatterns().size() - 1 );
		CSearchPattern* pSearchPattern( pSearchModel->GetPatternAt( index ) );

		m_searchPatternListBox.AddString( pSearchPattern->GetFilePath().GetPtr() );
	}
	m_searchPatternListBox.SetCurSel( index );

	SetDirty();
	OnLBnSelChange_SearchPattern();
	return true;
}

bool CAlbumSettingsDialog::DeleteSearchPattern( int index, bool doPrompt /*= false*/ )
{
	ASSERT( index != LB_ERR );
	if ( doPrompt )
		if ( IDCANCEL == AfxMessageBox( str::Format( IDS_DELETE_ATTR_PROMPT, m_model.GetSearchModel()->GetPatternAt( index )->GetFilePath().GetPtr() ).c_str(), MB_OKCANCEL ) )
			return false;

	m_model.RefSearchModel()->RemovePatternAt( index );
	m_searchPatternListBox.DeleteString( index );
	index = std::min( index, m_searchPatternListBox.GetCount() - 1 );
	m_searchPatternListBox.SetCurSel( index );

	SetDirty();
	OnLBnSelChange_SearchPattern();
	return true;
}

bool CAlbumSettingsDialog::MoveSearchPattern( seq::Direction moveBy )
{
	int selIndex = m_searchPatternListBox.GetCurSel();
	int newIndex = selIndex + moveBy;

	ASSERT( selIndex != LB_ERR && moveBy != 0 );
	m_searchPatternListBox.DeleteString( selIndex );

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	const CSearchPattern* pSearchPattern = pSearchModel->GetPatternAt( selIndex );

	seq::MoveBy( &pSearchModel->RefPatterns(), selIndex, moveBy );
	ENSURE( pSearchPattern == pSearchModel->GetPatternAt( newIndex ) );			// same pattern at new index

	if ( newIndex < m_searchPatternListBox.GetCount() )
		m_searchPatternListBox.InsertString( newIndex, pSearchPattern->GetFilePath().GetPtr() );
	else
		m_searchPatternListBox.AddString( pSearchPattern->GetFilePath().GetPtr() );

	VERIFY( m_searchPatternListBox.SetCurSel( newIndex ) != LB_ERR );
	SetDirty();
	OnLBnSelChange_SearchPattern();
	return true;
}

bool CAlbumSettingsDialog::AddSearchPattern( int index )
{
	ASSERT( index != LB_ERR );

	CSearchPatternDialog dlg( CSearchPattern(), this );
	if ( dlg.DoModal() != IDOK )
		return false;

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	int dupIndex = CheckForDuplicates( dlg.m_searchPattern.GetFilePath().GetPtr() );

	if ( dupIndex != -1 )
	{	// modify the same entry instead of
		index = dupIndex;
		pSearchModel->RemovePatternAt( index )->GetFilePath();
		m_searchPatternListBox.DeleteString( index );
	}

	pSearchModel->AddPattern( new CSearchPattern( dlg.m_searchPattern ), index );
	m_searchPatternListBox.InsertString( index, pSearchModel->GetPatternAt( index )->GetFilePath().GetPtr() );

	m_searchPatternListBox.SetCurSel( index );
	SetDirty();
	OnLBnSelChange_SearchPattern();
	return true;
}

bool CAlbumSettingsDialog::ModifySearchPattern( int index )
{
	ASSERT( index != LB_ERR );

	CSearchPatternDialog dlg( *m_model.GetSearchModel()->GetPatternAt( index ), this );
	if ( dlg.DoModal() != IDOK )
		return false;

	CSearchModel* pSearchModel = m_model.RefSearchModel();
	std::auto_ptr< CSearchPattern > pSearchPattern;
	int dupIndex = CheckForDuplicates( dlg.m_searchPattern.GetFilePath().GetPtr(), index );

	if ( dupIndex != -1 )
	{	// modify the same entry
		pSearchPattern = pSearchModel->RemovePatternAt( index );
		m_searchPatternListBox.DeleteString( index );
		index = dupIndex;
	}
	else
		pSearchPattern.reset( new CSearchPattern() );

	*pSearchPattern = dlg.m_searchPattern;

	m_searchPatternListBox.DeleteString( index );
	m_searchPatternListBox.InsertString( index, pSearchPattern->GetFilePath().GetPtr() );

	m_searchPatternListBox.SetCurSel( index );
	SetDirty();
	OnLBnSelChange_SearchPattern();
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
	int foundIndex = hlp::FindPathIndex( m_searchPatternListBox, pFilePath, ignoreIndex );
	if ( foundIndex != -1 )
		if ( AfxMessageBox( str::Format( IDS_REPLACE_DUP_PROMPT, pFilePath ).c_str(), MB_YESNO | MB_ICONQUESTION ) != IDYES )
			return -1;		// allow the duplicate to exist

	return foundIndex;
}

bool CAlbumSettingsDialog::SearchSourceFiles( void )
{
	CWaitCursor wait;
	bool success = true;

	ui::EnableControl( m_hWnd, IDCANCEL, false );

	fs::CFlexPath currFilePath;
	if ( m_pCaretFileAttr != NULL )
		currFilePath = m_pCaretFileAttr->GetPath();

	stdext::hash_set< fs::CFlexPath > oldFilePaths;
	utl::InsertFrom( std::inserter( oldFilePaths, oldFilePaths.end() ), m_model.GetImagesModel().GetFileAttrs(), func::ToFilePath() );

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
		m_pCaretFileAttr = NULL;

	m_newFilePaths.clear();
	const std::vector< CFileAttr* >& fileAttrs = m_model.GetImagesModel().GetFileAttrs();
	for ( std::vector< CFileAttr* >::const_iterator itFileAttr = fileAttrs.begin(); itFileAttr != fileAttrs.end(); ++itFileAttr )
		if ( oldFilePaths.find( ( *itFileAttr )->GetPath() ) == oldFilePaths.end() )
			m_newFilePaths.push_back( ( *itFileAttr )->GetPath() );

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
		currFilePath = m_imagesListCtrl.GetObjectAt< CFileAttr >( selCaretIndex )->GetPath();

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
	m_searchPatternListBox.ResetContent();

	const std::vector< CSearchPattern* >& searchPatterns = m_model.GetSearchModel()->GetPatterns();
	for ( std::vector< CSearchPattern* >::const_iterator itPattern = searchPatterns.begin(); itPattern != searchPatterns.end(); ++itPattern )
		m_searchPatternListBox.AddString( ( *itPattern )->GetFilePath().GetPtr() );

	m_searchPatternListBox.SetCurSel( 0 );
	OnLBnSelChange_SearchPattern();

	// setup list-control
	std::pair< ImagesColumn, bool > sortPair = ToListSortOrder( m_model.GetFileOrder() );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

	SetupFoundImagesListView();

	if ( !m_imagesListCtrl.AnySelected() )
		if ( m_pCaretFileAttr != NULL )
			m_imagesListCtrl.Select( m_pCaretFileAttr );

	m_thumbPreviewCtrl.SetImagePath( m_pCaretFileAttr != NULL ? m_pCaretFileAttr->GetPath() : fs::CFlexPath() );
}

void CAlbumSettingsDialog::InputAll( void )
{
	CSearchModel* pSearchModel = m_model.RefSearchModel();
	UINT maxFileCount = UINT_MAX;

	if ( IsDlgButtonChecked( IDC_MAX_FILE_COUNT_CHECK ) )
		if ( !m_maxFileCountEdit.ParseNumber( &maxFileCount ) )
			maxFileCount = UINT_MAX;

	pSearchModel->SetMaxFileCount( maxFileCount );

	Range< UINT > fileSizeRange = CSearchModel::s_anyFileSizeRange;

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

	m_model.StoreFileOrder( m_sortOrderCombo.GetEnum< fattr::Order >() );
	m_pCaretFileAttr = m_imagesListCtrl.GetSelected< CFileAttr >();
}

void CAlbumSettingsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_imagesListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_PATTERNS_LISTVIEW, m_patternsListCtrl );
	m_patternsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignRight | V_AlignCenter );

		DDX_Control( pDX, IDC_SEARCH_SPEC_MOVE_DOWN, m_moveDownButton );
		DDX_Control( pDX, IDC_SEARCH_SPEC_MOVE_UP, m_moveUpButton );
		DDX_Control( pDX, IDC_SEARCH_SPEC_LIST, m_searchPatternListBox );

	DDX_Control( pDX, IDC_MAX_FILE_COUNT_EDIT, m_maxFileCountEdit );
	DDX_Control( pDX, IDC_MIN_FILE_SIZE_EDIT, m_minSizeEdit );
	DDX_Control( pDX, IDC_MAX_FILE_SIZE_EDIT, m_maxSizeEdit );

	DDX_Control( pDX, IDC_LIST_ORDER_COMBO, m_sortOrderCombo );
	DDX_Control( pDX, IDC_FOUND_IMAGES_LISTVIEW, m_imagesListCtrl );
	DDX_Control( pDX, IDC_THUMB_PREVIEW_STATIC, m_thumbPreviewCtrl );
	DDX_Control( pDX, IDC_DOC_VERSION_LABEL, m_docVersionLabel );
	DDX_Control( pDX, IDC_DOC_VERSION_STATIC, m_docVersionStatic );
	m_imagesToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignRight | V_AlignCenter );
	DDX_Control( pDX, IDOK, m_okButton );

	if ( firstInit )
	{
		DragAcceptFiles();
		InitSymbolFont();

		m_moveUpButton.SetFont( &m_symbolFont );
		m_moveDownButton.SetFont( &m_symbolFont );
		m_imagesListCtrl.SetCompactIconSpacing();
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		OutputAll();
		SetDirty( m_model.MustAutoRegenerate() );		// set initial dirty if auto-generation is turned on (either explicit or by auto-drop)
	}
	else
		InputAll();

	CLayoutDialog::DoDataExchange( pDX );
}

BOOL CAlbumSettingsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	switch ( id )
	{
		case ID_EDIT_COPY:
			// special case: handled by the file list (since CDialog::OnCmdMsg() routes to the parent frame window)
			return m_imagesListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
	}

	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	if ( m_patternsEditor.OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	if ( m_imagesListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;

	return FALSE;
}

BOOL CAlbumSettingsDialog::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( m_patternsEditor.HandleTranslateMessage( pMsg ) )
			return true;

	return CLayoutDialog::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumSettingsDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
	ON_CBN_SELCHANGE( IDC_LIST_ORDER_COMBO, OnCBnSelChange_SortOrder )
	ON_BN_CLICKED( IDC_MAX_FILE_COUNT_CHECK, OnToggle_MaxFileCount )
	ON_BN_CLICKED( IDC_MIN_FILE_SIZE_CHECK, OnToggle_MinSize )
	ON_BN_CLICKED( IDC_MAX_FILE_SIZE_CHECK, OnToggle_MaxSize )
	ON_EN_CHANGE( IDC_MAX_FILE_COUNT_EDIT, OnEnChange_MaxFileCount )
	ON_EN_CHANGE( IDC_MIN_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_EN_CHANGE( IDC_MAX_FILE_SIZE_EDIT, OnEnChange_MinMaxSize )
	ON_BN_CLICKED( IDC_AUTO_REGENERATE_CHECK, OnToggle_AutoRegenerate )
	ON_BN_CLICKED( IDC_AUTO_DROP_CHECK, OnToggle_AutoDrop )

	ON_LBN_SELCHANGE( IDC_SEARCH_SPEC_LIST, OnLBnSelChange_SearchPattern )
	ON_LBN_DBLCLK( IDC_SEARCH_SPEC_LIST, OnLBnDblclk_SearchPattern )
	ON_BN_CLICKED( IDC_SEARCH_SPEC_MOVE_UP, On_MoveUp_SearchPattern )
	ON_BN_CLICKED( IDC_SEARCH_SPEC_MOVE_DOWN, On_MoveDown_SearchPattern )
	ON_BN_CLICKED( ID_ADD_ITEM, OnAdd_SearchPattern )
	ON_BN_CLICKED( ID_EDIT_ITEM, OnModify_SearchPattern )
	ON_BN_CLICKED( ID_REMOVE_ITEM, OnDelete_SearchPattern )

	ON_BN_CLICKED( IDC_SEARCH_FOR_FILES, OnSearchSourceFiles )
	ON_COMMAND_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, On_OrderRandomShuffle )
	ON_UPDATE_COMMAND_UI_RANGE( ID_ORDER_ORIGINAL, ID_ORDER_BY_DIMENSION_DESC, OnUpdate_OrderRandomShuffle )
	ON_NOTIFY( LVN_COLUMNCLICK, IDC_FOUND_IMAGES_LISTVIEW, OnLVnColumnClick_FoundImages )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FOUND_IMAGES_LISTVIEW, OnLVnItemChanged_FoundImages )
	ON_NOTIFY( LVN_GETDISPINFO, IDC_FOUND_IMAGES_LISTVIEW, OnLVnGetDispInfo_FoundImages )
	ON_CONTROL( lv::LVN_ItemsReorder, IDC_FOUND_IMAGES_LISTVIEW, OnLVnItemsReorder_FoundImages )
	ON_STN_DBLCLK( IDC_THUMB_PREVIEW_STATIC, OnStnDblClk_ThumbPreviewStatic )

	// image file operations: CM_OPEN_IMAGE_FILE, CM_DELETE_FILE, CM_DELETE_FILE_NO_UNDO, CM_MOVE_FILE, CM_EXPLORE_IMAGE
	ON_COMMAND_RANGE( CM_OPEN_IMAGE_FILE, CM_EXPLORE_IMAGE, OnImageFileOp )
END_MESSAGE_MAP()

void CAlbumSettingsDialog::OnDestroy( void )
{
	CLayoutDialog::OnDestroy();
}

void CAlbumSettingsDialog::OnOK( void )
{
	if ( !UpdateData( DialogSaveChanges ) )
		TRACE( _T("UpdateData failed during dialog termination.\n") );		// UpdateData() will set focus to correct item
	else if ( m_isDirty )
		SearchSourceFiles();
	else
		CLayoutDialog::OnOK();
}

void CAlbumSettingsDialog::OnCancel( void )
{
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
		DropSearchPattern( fs::CFlexPath( filePath ) );
	}
	::DragFinish( hDropInfo );
}

void CAlbumSettingsDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
	if ( pWnd == &m_imagesListCtrl )
	{
		static CMenu contextMenu;
		if ( NULL == (HMENU)contextMenu )
			ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AlbumFoundListPopup );

		if ( m_imagesListCtrl.GetSelectedCount() != 0 )
			if ( (HMENU)contextMenu != NULL )
				contextMenu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
	}
}

void CAlbumSettingsDialog::OnCBnSelChange_SortOrder( void )
{
	fattr::Order selFileOrder = m_sortOrderCombo.GetEnum< fattr::Order >();

	std::pair< ImagesColumn, bool > sortPair = ToListSortOrder( selFileOrder );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );

	m_model.ModifyFileOrder( selFileOrder );
	SetupFoundImagesListView();
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

void CAlbumSettingsDialog::OnLBnSelChange_SearchPattern( void )
{
	int selIndex = m_searchPatternListBox.GetCurSel();
	int attrCount = static_cast<int>( m_model.GetSearchModel()->GetPatterns().size() );

	ASSERT( selIndex != LB_ERR || 0 == m_searchPatternListBox.GetCount() );
	ui::EnableWindow( m_moveUpButton, selIndex > 0 );
	ui::EnableWindow( m_moveDownButton, selIndex < attrCount - 1 );

	ui::EnableControl( m_hWnd, ID_EDIT_ITEM, selIndex != LB_ERR );
	ui::EnableControl( m_hWnd, ID_REMOVE_ITEM, selIndex != LB_ERR );

	CheckDlgButton( IDC_AUTO_REGENERATE_CHECK, GetCheckStateAutoRegen() );
	CheckDlgButton( IDC_AUTO_DROP_CHECK, m_model.IsAutoDropRecipient() );
}

void CAlbumSettingsDialog::OnLBnDblclk_SearchPattern( void )
{
	if ( m_searchPatternListBox.GetCount() > 0 )
		OnModify_SearchPattern();
}

void CAlbumSettingsDialog::On_MoveUp_SearchPattern( void )
{
	MoveSearchPattern( seq::Prev );
}

void CAlbumSettingsDialog::On_MoveDown_SearchPattern( void )
{
	MoveSearchPattern( seq::Next );
}

void CAlbumSettingsDialog::OnAdd_SearchPattern( void )
{
	int attrCount = static_cast<int>( m_model.GetSearchModel()->GetPatterns().size() );
	int atIndex = m_searchPatternListBox.GetCurSel();

	if ( LB_ERR == atIndex )
		if ( 0 == attrCount )
			atIndex = 0;
		else
			ASSERT( false );

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		atIndex = attrCount;

	AddSearchPattern( atIndex );
}

void CAlbumSettingsDialog::OnModify_SearchPattern( void )
{
	ModifySearchPattern( m_searchPatternListBox.GetCurSel() );
}

void CAlbumSettingsDialog::OnDelete_SearchPattern( void )
{
	bool doPrompt = !ui::IsKeyPressed( VK_SHIFT );
	DeleteSearchPattern( m_searchPatternListBox.GetCurSel(), doPrompt );
}

void CAlbumSettingsDialog::OnSearchSourceFiles( void )
{
	SearchSourceFiles();
}

void CAlbumSettingsDialog::On_OrderRandomShuffle( UINT cmdId )
{
	fattr::Order fileOrder = hlp::OrderOfCmd( cmdId );

	m_sortOrderCombo.SetValue( fileOrder );
	m_model.ModifyFileOrder( fileOrder );
	SetupFoundImagesListView();
}

void CAlbumSettingsDialog::OnUpdate_OrderRandomShuffle( CCmdUI* pCmdUI )
{
	fattr::Order fileOrder = hlp::OrderOfCmd( pCmdUI->m_nID );

	pCmdUI->SetCheck( fileOrder == m_model.GetFileOrder() );
}

void CAlbumSettingsDialog::OnImageFileOp( UINT cmdId )
{
	std::vector< std::tstring > targetFiles;
	m_imagesListCtrl.QuerySelectedItemPaths( targetFiles );
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
				const CFileAttr* pFileAttr = CReportListControl::AsPtr< CFileAttr >( pDisplayInfo->item.lParam );
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
	std::vector< CFileAttr* > customSequence;
	customSequence.reserve( m_model.GetFileAttrCount() );

	for ( int i = 0, count = m_imagesListCtrl.GetItemCount(); i != count; ++i )
		customSequence.push_back( m_imagesListCtrl.GetPtrAt< CFileAttr >( i ) );

	m_model.SetCustomOrderSequence( customSequence );

	// update UI
	fattr::Order fileOrder = m_model.GetFileOrder();
	m_sortOrderCombo.SetValue( fileOrder );
	std::pair< ImagesColumn, bool > sortPair = ToListSortOrder( fileOrder );
	m_imagesListCtrl.SetSortByColumn( sortPair.first, sortPair.second );
}

void CAlbumSettingsDialog::OnStnDblClk_ThumbPreviewStatic( void )
{
	shell::Execute( this, m_thumbPreviewCtrl.GetImagePath().GetPtr() );
}
