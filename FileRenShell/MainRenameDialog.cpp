
#include "stdafx.h"
#include "MainRenameDialog.h"
#include "Application.h"
#include "FileWorkingSet.h"
#include "FileSetUi.h"
#include "MenuCommand.h"
#include "PathFunctors.h"
#include "ReplaceDialog.h"
#include "CapitalizeOptionsDialog.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/MenuUtilities.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/UtilitiesEx.h"
#include "utl/VisualTheme.h"
#include "utl/resource.h"


static bool dbgGuides = false;
static bool useDefaultDraw = false;

enum CustomColors { ColorDeletedText = color::Red, ColorModifiedText = color::Blue, ColorErrorBk = color::PastelPink };


namespace reg
{
	static const TCHAR section_mainDialog[] = _T("Main Dialog");
	static const TCHAR entry_formatHistory[] = _T("Format History");
	static const TCHAR entry_autoGenerate[] = _T("Auto Generate");
	static const TCHAR entry_seqCount[] = _T("Sequence Count");
	static const TCHAR entry_seqCountAutoAdvance[] = _T("Sequence Count Auto Advance");
	static const TCHAR entry_changeCase[] = _T("Change Case");
	static const TCHAR entry_delimiterSetHistory[] = _T("Delimiter Set History");
	static const TCHAR entry_newDelimiterHistory[] = _T("New Delimiter History");
}

static const TCHAR defaultDelimiterSet[] = _T(".;-_ \t");
static const TCHAR defaultNewDelimiter[] = _T(" ");
static const TCHAR specialSep[] = _T("\n");


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FORMAT_COMBO, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_FILE_RENAME_LIST, Size },

		{ IDC_SOURCE_FILES_GROUP, MoveY },
		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },

		{ IDC_DESTINATION_FILES_GROUP, Move },
		{ IDC_PASTE_FILES_BUTTON, Move },
		{ IDC_CLEAR_FILES_BUTTON, Move },
		{ IDC_CAPITALIZE_BUTTON, Move },
		{ IDC_CHANGE_CASE_BUTTON, Move },

		{ IDC_REPLACE_FILES_BUTTON, Move },
		{ IDC_REPLACE_DELIMS_BUTTON, Move },
		{ IDC_DELIMITER_SET_COMBO, Move },
		{ IDC_DELIMITER_STATIC, Move },
		{ IDC_NEW_DELIMITER_EDIT, Move },
		{ IDC_PICK_RENAME_ACTIONS, Move },

		{ IDOK, MoveX },
		{ IDC_UNDO_BUTTON, MoveX },
		{ IDCANCEL, MoveX }
	};
}


CMainRenameDialog::CMainRenameDialog( MenuCommand menuCmd, CFileWorkingSet* pFileData, CWnd* pParent )
	: CBaseMainDialog( IDD_RENAME_FILES_DIALOG, pParent )
	, m_menuCmd( menuCmd )
	, m_pFileData( pFileData )
	, m_autoGenerate( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, false ) != FALSE )
	, m_seqCountAutoAdvance( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, true ) != FALSE )
	, m_mode( Uninit )
	, m_formatCombo( ui::HistoryMaxSize, specialSep )
	, m_fileListView( IDC_FILE_RENAME_LIST, LVS_EX_GRIDLINES | CReportListControl::DefaultStyleEx )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimiterSetCombo( ui::HistoryMaxSize, specialSep )
	, m_delimStatic( CThemeItem( L"EXPLORERBAR", vt::EBP_IEBARMENU, vt::EBM_NORMAL ) )
	, m_pickRenameActionsStatic( ui::DropDown )
{
	ASSERT_PTR( m_pFileData );
	m_regSection = reg::section_mainDialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	app::GetLogger().m_logFileMaxSize = -1;		// unlimited

	m_changeCaseButton.SetSelValue( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_changeCase, ExtLowerCase ) );

	m_formatToolbar.GetStrip()
		.AddButton( ID_PICK_FORMAT_TOKEN )
		.AddButton( ID_PICK_DIR_PATH )
		.AddButton( ID_PICK_TEXT_TOOLS )
		.AddButton( ID_TOGGLE_AUTO_GENERATE );

	m_seqCountToolbar.GetStrip()
		.AddButton( ID_SEQ_COUNT_RESET )
		.AddButton( ID_SEQ_COUNT_FIND_NEXT )
		.AddButton( ID_SEQ_COUNT_AUTO_ADVANCE );

	//m_fileListView.SetUseExplorerTheme( false );			// ...testing
}

CMainRenameDialog::~CMainRenameDialog()
{
	utl::ClearOwningContainer( m_displayItems );
}

void CMainRenameDialog::SetupFileListView( void )
{
	int orgSel = m_fileListView.GetCurSel();

	m_fileListView.AddInternalChange();

	m_fileListView.SetRedraw( FALSE );
	m_fileListView.DeleteAllItems();

	utl::ClearOwningContainer( m_displayItems );

	const fs::PathPairMap& renamePairs = m_pFileData->GetRenamePairs();
	int pos = 0;

	for ( fs::PathPairMap::const_iterator it = renamePairs.begin();
		  it != renamePairs.end(); ++it, ++pos )
	{
		CDisplayItem* pItem = new CDisplayItem( it->first, it->second );
		m_displayItems.push_back( pItem );

		m_fileListView.InsertItem( LVIF_TEXT | LVIF_PARAM, pos, (LPTSTR)pItem->m_srcFnameExt.c_str(), 0, 0, 0, (LPARAM)pItem );		// Source

		//if ( !it->second.IsEmpty() )
		//	if ( pItem->m_match == str::MatchEqual )	// don't set the sub-item text if custom drawn, since is hard to erase the text printed by default
		m_fileListView.SetItemText( pos, Destination, pItem->m_destFnameExt.c_str() );											// Destination
	}

	m_fileListView.ReleaseInternalChange();

	m_fileListView.SetRedraw( TRUE );
	m_fileListView.Invalidate();
	m_fileListView.UpdateWindow();

	// restore the selection (if any)
	if ( orgSel != -1 )
	{
		m_fileListView.EnsureVisible( orgSel, FALSE );
		m_fileListView.SetCurSel( orgSel );
	}
}

int CMainRenameDialog::FindItemPos( const fs::CPath& sourcePath ) const
{
	for ( unsigned int pos = 0; pos != m_displayItems.size(); ++pos )
		if ( m_displayItems[ pos ]->m_srcPath.Equal( sourcePath ) )
			return pos;

	return -1;
}

void CMainRenameDialog::SwitchMode( Mode mode )
{
	m_mode = mode;

	static const std::vector< std::tstring > labels = str::LoadStrings( IDS_OK_BUTTON_LABELS );	// &Make Names|Rena&me|R&ollback
	ui::SetWindowText( ::GetDlgItem( m_hWnd, IDOK ), labels[ m_mode ] );

	static const UINT ctrlIds[] =
	{
		IDC_FORMAT_COMBO, IDC_STRIP_BAR_1, IDC_STRIP_BAR_2, IDC_COPY_SOURCE_PATHS_BUTTON,
		IDC_PASTE_FILES_BUTTON, IDC_CLEAR_FILES_BUTTON, IDC_CAPITALIZE_BUTTON, IDC_CHANGE_CASE_BUTTON,
		IDC_REPLACE_FILES_BUTTON, IDC_REPLACE_DELIMS_BUTTON, IDC_DELIMITER_SET_COMBO, IDC_NEW_DELIMITER_EDIT
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), m_mode != UndoRollbackMode );
	ui::EnableControl( *this, IDC_UNDO_BUTTON, m_mode != UndoRollbackMode && m_pFileData->CanUndo() );
}

void CMainRenameDialog::PostMakeDest( bool silent /*= false*/ )
{
	m_pBatchTransaction.reset();

	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	SetupFileListView();							// fill in and select the found files list

	try
	{
		m_pFileData->CheckPathCollisions();			// throws if duplicates are found
		SwitchMode( RenameMode );
	}
	catch ( CRuntimeException& e )
	{
		if ( m_pFileData->HasErrors() )
		{
			m_fileListView.EnsureVisible( (int)*m_pFileData->GetErrorIndexes().begin(), FALSE );
			m_fileListView.Invalidate();
		}
		SwitchMode( MakeMode );

		if ( !silent )
			e.ReportError();
	}
}

void CMainRenameDialog::ListItem_DrawTextDiffs( const NMLVCUSTOMDRAW* pCustomDraw )
{
	ASSERT_PTR( pCustomDraw );

	ListItem_DrawTextDiffs( CDC::FromHandle( pCustomDraw->nmcd.hdc ),
		MakeItemTextRect( pCustomDraw ),
		static_cast< int >( pCustomDraw->nmcd.dwItemSpec ),
		static_cast< Column >( pCustomDraw->iSubItem ) );

	if ( Destination == pCustomDraw->iSubItem )
	{
		if ( HasFlag( pCustomDraw->nmcd.uItemState, CDIS_FOCUS ) )
		{
			CRect focusRect;
			m_fileListView.GetItemRect( static_cast< int >( pCustomDraw->nmcd.dwItemSpec ), &focusRect, LVIR_BOUNDS );

			if ( m_fileListView.UseExplorerTheme() )
				focusRect.DeflateRect( 1, 1 );
			else
				focusRect.DeflateRect( 4, 0, 1, 1 );

			::DrawFocusRect( pCustomDraw->nmcd.hdc, &focusRect );
		}
	}
}

void CMainRenameDialog::ListItem_DrawTextDiffs( CDC* pDC, const CRect& textRect, int itemIndex, Column column )
{
	enum { TextStyle = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX };	// DT_NOCLIP

	const CDisplayItem* pItem = m_displayItems[ itemIndex ];

	if ( dbgGuides )
		ui::FrameRect( *pDC, textRect, Source == column ? color::Lime : color::Orange );

	if ( Destination == column )
		if ( m_pFileData->IsErrorAt( itemIndex ) )
		{
			CRect rect = textRect;
			rect.InflateRect( 2, 0 );
			ui::FillRect( *pDC, rect, ColorErrorBk );
		}

	// with "Explorer" visual theme background is bright (white) for selected and unselected state -> ignore selected state to keep text colour dark
	bool selInvert = ListItem_IsSelectedInvert( itemIndex );

	COLORREF stdTextColor = GetSysColor( selInvert ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT );		// if not focused, selection is light gray in background and normal in text

	if ( Destination == column )
		if ( str::MatchEqual == pItem->m_match )
			stdTextColor = GetSysColor( COLOR_GRAYTEXT );		// gray-out text of unmodified dest files

	COLORREF oldTextColor = pDC->SetTextColor( stdTextColor );
	CFont* pOriginalFont = pDC->GetCurrentFont();

	const TCHAR* pText = Source == column ? pItem->m_srcFnameExt.c_str() : pItem->m_destFnameExt.c_str();
	CRect itemRect = textRect;
	COLORREF highlightColor = Source == column ? ColorDeletedText : ColorModifiedText;

	const std::vector< str::Match >& matchSeq = Source == column ? pItem->m_srcMatchSeq : pItem->m_destMatchSeq;

	if ( str::MatchEqual == pItem->m_match || matchSeq.empty() )
		pDC->DrawText( pText, -1, &itemRect, TextStyle );			// straight text without highlighting
	else
		for ( size_t i = 0, size = matchSeq.size(); i != size && itemRect.left < itemRect.right; )
		{
			unsigned int matchLen = 1;		// skip current pos

			while ( i + matchLen != size && matchSeq[ i ] == matchSeq[ i + matchLen ] )
				++matchLen;

			pDC->SetTextColor( str::MatchEqual == matchSeq[ i ] || selInvert ? stdTextColor : highlightColor );
			if ( str::MatchNotEqual == matchSeq[ i ] )
				SelectBoldFont( pDC );
			else
				pDC->SelectObject( pOriginalFont );

			pDC->DrawText( pText, matchLen, &itemRect, TextStyle );

			itemRect.left += pDC->GetTextExtent( pText, matchLen ).cx;

			pText += matchLen;
			i += matchLen;
		}

	pDC->SetTextColor( oldTextColor );
	pDC->SelectObject( pOriginalFont );
}

CRect CMainRenameDialog::MakeItemTextRect( const NMLVCUSTOMDRAW* pCustomDraw )
{
	CRect textRect = pCustomDraw->nmcd.rc;

	// requires larger spacing for sub-item: 2 for item, 6 for sub-item
	textRect.left += Source == pCustomDraw->iSubItem ? CReportListControl::ItemSpacingX : CReportListControl::SubItemSpacingX;
	return textRect;
}

bool CMainRenameDialog::ListItem_FillBkgnd( NMLVCUSTOMDRAW* pCustomDraw ) const
{
	ASSERT_PTR( pCustomDraw );

	const CDisplayItem* pItem = reinterpret_cast< const CDisplayItem* >( pCustomDraw->nmcd.lItemlParam );
	int itemIndex = static_cast< int >( pCustomDraw->nmcd.dwItemSpec );

	COLORREF bkColor = CLR_NONE;
	if ( !m_fileListView.UseExplorerTheme() && m_fileListView.HasItemState( itemIndex, LVIS_SELECTED ) )
		bkColor = GetSysColor( ::GetFocus() == m_fileListView.m_hWnd ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION );
	else if ( m_pBatchTransaction.get() != NULL && m_pBatchTransaction->ContainsError( pItem->m_srcPath ) )
		bkColor = ColorErrorBk;									// item with error
	else if ( itemIndex & 0x01 )
		bkColor = color::GhostWhite;							// alternate row background

	if ( CLR_NONE == bkColor )
		return false;

	ui::FillRect( pCustomDraw->nmcd.hdc, pCustomDraw->nmcd.rc, bkColor );
	pCustomDraw->clrTextBk = bkColor;							// just in case not using CDRF_SKIPDEFAULT
	return true;
}

bool CMainRenameDialog::ListItem_IsSelectedInvert( int itemIndex ) const
{
	// with "Explorer" visual theme background is bright (white) for selected and unselected state -> ignore selected state to keep text colour dark
	if ( !m_fileListView.UseExplorerTheme() )
		return m_fileListView.HasItemState( itemIndex, LVIS_SELECTED ) && ::GetFocus() == m_fileListView.m_hWnd;

	return false;
}

CFont* CMainRenameDialog::SelectBoldFont( CDC* pDC )
{
	static CFont boldFont;

	if ( (HFONT)boldFont == NULL )
	{
		LOGFONT logFont;
		pDC->GetCurrentFont()->GetLogFont( &logFont );
		logFont.lfWeight = FW_BOLD;
		boldFont.CreateFontIndirect( &logFont );
	}

	return pDC->SelectObject( &boldFont );
}

CWnd* CMainRenameDialog::GetWnd( void )
{
	return this;
}

CLogger* CMainRenameDialog::GetLogger( void )
{
	return &app::GetLogger();
}

fs::UserFeedback CMainRenameDialog::HandleFileError( const fs::CPath& sourcePath, const std::tstring& message )
{
	int pos = FindItemPos( sourcePath );
	if ( pos != -1 )
		m_fileListView.EnsureVisible( pos, FALSE );
	m_fileListView.Invalidate();

	switch ( AfxMessageBox( message.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
	{
		default: ASSERT( false );
		case IDIGNORE:	return fs::Ignore;
		case IDRETRY:	return fs::Retry;
		case IDABORT:	return fs::Abort;
	}
}

void CMainRenameDialog::AutoGenerateFiles( void )
{
	std::tstring renameFormat = m_formatCombo.GetCurrentText();
	UINT seqCount = m_seqCountEdit.GetNumericValue();
	bool succeeded = m_pFileData->GenerateDestPaths( renameFormat, &seqCount );

	if ( !succeeded )
		m_pFileData->ClearDestinationFiles();

	PostMakeDest( true );							// silent mode, no modal messages
	m_formatCombo.SetFrameColor( succeeded ? CLR_NONE : color::Error );
}

bool CMainRenameDialog::RenameFiles( void )
{
	m_pBatchTransaction.reset( new fs::CBatchRename( this ) );
	if ( !m_pBatchTransaction->Rename( m_pFileData->GetRenamePairs() ) )
		return false;

	return true;
}

bool CMainRenameDialog::ChangeSeqCount( UINT seqCount )
{
	if ( m_seqCountEdit.GetNumber< UINT >() == seqCount )
		return false;

	m_seqCountEdit.SetNumericValue( seqCount );
	OnEnChange_SeqCounter();
	return true;
}

void CMainRenameDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* /*pTooltip*/ ) const
{
	switch ( cmdId )
	{
		case IDC_CHANGE_CASE_BUTTON:
		{
			static const std::vector< std::tstring > tooltips = str::LoadStrings( IDC_CHANGE_CASE_BUTTON );
			ChangeCase changeCase = m_changeCaseButton.GetSelEnum< ChangeCase >();
			rText = tooltips.at( changeCase );
			break;
		}
		case IDC_CAPITALIZE_BUTTON:
			if ( m_capitalizeButton.GetRhsPartRect().PtInRect( ui::GetCursorPos( m_capitalizeButton ) ) )
			{
				static const std::tstring details = str::Load( IDI_DETAILS );
				rText = details;
			}
			break;
		case IDC_REPLACE_FILES_BUTTON:
			rText = CReplaceDialog::FormatTooltipMessage();
			break;
	}
}

void CMainRenameDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );
	m_formatToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_SEQ_COUNT_EDIT, m_seqCountEdit );
	m_seqCountToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListView );
	DDX_Control( pDX, IDC_CAPITALIZE_BUTTON, m_capitalizeButton );
	DDX_Control( pDX, IDC_CHANGE_CASE_BUTTON, m_changeCaseButton );
	DDX_Control( pDX, IDC_DELIMITER_SET_COMBO, m_delimiterSetCombo );
	DDX_Control( pDX, IDC_NEW_DELIMITER_EDIT, m_newDelimiterEdit );
	DDX_Control( pDX, IDC_DELIMITER_STATIC, m_delimStatic );
	DDX_Control( pDX, IDC_PICK_RENAME_ACTIONS, m_pickRenameActionsStatic );
	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS );
	ui::DDX_ButtonIcon( pDX, IDC_REPLACE_FILES_BUTTON, ID_EDIT_REPLACE );
	ui::DDX_ButtonIcon( pDX, IDC_UNDO_BUTTON, ID_EDIT_UNDO );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( Uninit == m_mode )
		{
			m_formatCombo.LimitText( _MAX_PATH );
			CTextEdit::SetFixedFont( &m_delimiterSetCombo );
			m_delimiterSetCombo.LimitText( 64 );
			m_newDelimiterEdit.LimitText( 64 );

			m_formatCombo.LoadHistory( m_regSection.c_str(), reg::entry_formatHistory, _T("## - *.*") );
			m_delimiterSetCombo.LoadHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory, defaultDelimiterSet );
			m_newDelimiterEdit.SetWindowText( AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, defaultNewDelimiter ) );

			m_seqCountEdit.SetNumericValue( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_seqCount, 1 ) );
			SetupFileListView();			// fill in and select the found files list
			SwitchMode( MakeMode );
		}
	}

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainRenameDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_CBN_EDITCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_CBN_SELCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_COMMAND( ID_PICK_FORMAT_TOKEN, OnPickFormatToken )
	ON_COMMAND( ID_PICK_DIR_PATH, OnPickDirPath )
	ON_COMMAND_RANGE( IDC_PICK_DIR_PATH_BASE, IDC_PICK_DIR_PATH_MAX, OnDirPathPicked )
	ON_COMMAND( ID_PICK_TEXT_TOOLS, OnPickTextTools )
	ON_COMMAND_RANGE( ID_TEXT_TITLE_CASE, ID_TEXT_SPACE_TO_UNDERBAR, OnFormatTextToolPicked )
	ON_COMMAND( ID_TOGGLE_AUTO_GENERATE, OnToggleAutoGenerate )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_AUTO_GENERATE, OnUpdateAutoGenerate )
	ON_COMMAND_RANGE( ID_NUMERIC_SEQUENCE_2DIGITS, ID_EXTENSION_SEPARATOR, OnNumericSequence )
	ON_BN_CLICKED( IDC_UNDO_BUTTON, OnBnClicked_Undo )
	ON_EN_CHANGE( IDC_SEQ_COUNT_EDIT, OnEnChange_SeqCounter )
	ON_COMMAND( ID_SEQ_COUNT_RESET, OnSeqCountReset )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_RESET, OnUpdateSeqCountReset )
	ON_COMMAND( ID_SEQ_COUNT_FIND_NEXT, OnSeqCountFindNext )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_FIND_NEXT, OnUpdateSeqCountFindNext )
	ON_COMMAND( ID_SEQ_COUNT_AUTO_ADVANCE, OnSeqCountAutoAdvance )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_AUTO_ADVANCE, OnUpdateSeqCountAutoAdvance )
	ON_NOTIFY( NM_CUSTOMDRAW, IDC_FILE_RENAME_LIST, OnCustomDrawFileRenameList )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestFiles )
	ON_BN_CLICKED( IDC_CLEAR_FILES_BUTTON, OnBnClicked_ClearDestFiles )
	ON_BN_CLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeDestFiles )
	ON_SBN_RIGHTCLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeOptions )
	ON_BN_CLICKED( IDC_CHANGE_CASE_BUTTON, OnBnClicked_ChangeCase )
	ON_BN_CLICKED( IDC_REPLACE_FILES_BUTTON, OnBnClicked_ReplaceDestFiles )
	ON_BN_CLICKED( IDC_REPLACE_DELIMS_BUTTON, OnBnClicked_ReplaceAllDelimitersDestFiles )
	ON_CBN_EDITCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_CBN_SELCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_EN_CHANGE( IDC_NEW_DELIMITER_EDIT, OnFieldChanged )
	ON_BN_CLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_BN_DOUBLECLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_COMMAND( ID_SINGLE_WHITESPACE, OnSingleWhitespace )
	ON_COMMAND( ID_REMOVE_WHITESPACE, OnRemoveWhitespace )
	ON_COMMAND( ID_DASH_TO_SPACE, OnDashToSpace )
	ON_COMMAND( ID_SPACE_TO_DASH, OnSpaceToDash )
	ON_COMMAND( ID_UNDERBAR_TO_SPACE, OnUnderbarToSpace )
	ON_COMMAND( ID_SPACE_TO_UNDERBAR, OnSpaceToUnderbar )
	ON_COMMAND( ID_ENSURE_UNIFORM_NUM_PADDING, OnEnsureUniformNumPadding )
END_MESSAGE_MAP()

BOOL CMainRenameDialog::OnInitDialog( void )
{
	BOOL defaultFocus = CBaseMainDialog::OnInitDialog();
	if ( m_autoGenerate )
		AutoGenerateFiles();

	UINT cmdId = 0, flashId = 0;

	switch ( m_menuCmd )
	{
		case Cmd_RenameAndCopy:				cmdId = flashId = IDC_COPY_SOURCE_PATHS_BUTTON; break;
		case Cmd_RenameAndCapitalize:		cmdId = flashId = IDC_CAPITALIZE_BUTTON; break;
		case Cmd_RenameAndLowCaseExt:		cmdId = flashId = IDC_CHANGE_CASE_BUTTON; m_changeCaseButton.SetSelValue( ExtLowerCase ); break;
		case Cmd_RenameAndReplace:			cmdId = flashId = IDC_REPLACE_FILES_BUTTON; break;
		case Cmd_RenameAndReplaceDelims:	cmdId = flashId = IDC_REPLACE_DELIMS_BUTTON; break;
		case Cmd_RenameAndSingleWhitespace:	cmdId = ID_SINGLE_WHITESPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case Cmd_RenameAndRemoveWhitespace:	cmdId = ID_REMOVE_WHITESPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case Cmd_RenameAndDashToSpace:		cmdId = ID_DASH_TO_SPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case Cmd_RenameAndUnderbarToSpace:	cmdId = ID_UNDERBAR_TO_SPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case Cmd_RenameAndSpaceToUnderbar:	cmdId = ID_SPACE_TO_UNDERBAR; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case Cmd_UndoRename:				cmdId = flashId = IDC_UNDO_BUTTON; break;
	}

	if ( cmdId != 0 )
	{
		PostMessage( WM_COMMAND, MAKEWPARAM( cmdId, BN_CLICKED ), 0 );
		if ( CWnd* pCtrl = GetDlgItem( cmdId ) )
			GotoDlgCtrl( pCtrl );
		defaultFocus = FALSE;
	}

	if ( flashId != 0 )
		if ( CWnd* pCtrl = GetDlgItem( flashId ) )
			ui::FlashCtrlFrame( pCtrl );

	return defaultFocus;
}

void CMainRenameDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case MakeMode:
		{
			std::tstring renameFormat = m_formatCombo.GetCurrentText();
			UINT oldSeqCount = m_seqCountEdit.GetNumericValue(), newSeqCount = oldSeqCount;

			if ( m_pFileData->GenerateDestPaths( renameFormat, &newSeqCount ) )
			{
				if ( !m_seqCountAutoAdvance )
					newSeqCount = oldSeqCount;

				if ( newSeqCount != oldSeqCount )
					m_seqCountEdit.SetNumericValue( newSeqCount );

				PostMakeDest();
			}
			else
			{
				AfxMessageBox( str::Format( IDS_INVALID_FORMAT, renameFormat.c_str() ).c_str(), MB_ICONERROR | MB_OK );
				ui::TakeFocus( m_formatCombo );
			}
			break;
		}
		case RenameMode:
			if ( RenameFiles() )
			{
				m_pFileData->SaveUndoInfo( m_pBatchTransaction->GetRenamedKeys() );

				m_formatCombo.SaveHistory( m_regSection.c_str(), reg::entry_formatHistory );
				m_delimiterSetCombo.SaveHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory );
				AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, ui::GetWindowText( &m_newDelimiterEdit ).c_str() );
				AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_seqCount, m_seqCountEdit.GetNumericValue() );

				CBaseMainDialog::OnOK();
			}
			else
			{
				m_pFileData->SaveUndoInfo( m_pBatchTransaction->GetRenamedKeys() );
				SwitchMode( MakeMode );
			}
			break;
		case UndoRollbackMode:
			if ( RenameFiles() )
			{
				m_pFileData->CommitUndoInfo();
				CBaseMainDialog::OnOK();
			}
			else
				SwitchMode( MakeMode );
			break;
	}
}

void CMainRenameDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, m_autoGenerate );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, m_seqCountAutoAdvance );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_changeCase, m_changeCaseButton.GetSelValue() );

	CBaseMainDialog::OnDestroy();
}

void CMainRenameDialog::OnChanged_Format( void )
{
	OnFieldChanged();

	CPathFormatter formatter( m_formatCombo.GetCurrentText() );
	m_formatCombo.SetFrameColor( formatter.IsValidFormat() ? CLR_NONE : color::Error );

	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CMainRenameDialog::OnSeqCountReset( void )
{
	ChangeSeqCount( 1 );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CMainRenameDialog::OnUpdateSeqCountReset( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_seqCountEdit.GetNumericValue() != 1 );
}

void CMainRenameDialog::OnSeqCountFindNext( void )
{
	UINT seqCount = m_pFileData->FindNextAvailSeqCount( m_formatCombo.GetCurrentText() );
	ChangeSeqCount( seqCount );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CMainRenameDialog::OnUpdateSeqCountFindNext( CCmdUI* pCmdUI )
{
	CPathFormatter format( m_formatCombo.GetCurrentText() );
	pCmdUI->Enable( format.m_isNumericFormat );
}

void CMainRenameDialog::OnSeqCountAutoAdvance( void )
{
	m_seqCountAutoAdvance = !m_seqCountAutoAdvance;
}

void CMainRenameDialog::OnUpdateSeqCountAutoAdvance( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_seqCountAutoAdvance );
}

void CMainRenameDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !m_pFileData->CopyClipSourcePaths( FilenameExt, this ) )
		AfxMessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CMainRenameDialog::OnBnClicked_ClearDestFiles( void )
{
	m_pFileData->ClearDestinationFiles();

	SetupFileListView();	// fill in and select the found files list
	SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PasteDestFiles( void )
{
	try
	{
		m_pFileData->PasteClipDestinationPaths( this );
		PostMakeDest();
	}
	catch ( CRuntimeException& e )
	{
		e.ReportError();
	}
}

void CMainRenameDialog::OnBnClicked_CapitalizeDestFiles( void )
{
	CTitleCapitalizer capitalizer;
	m_pFileData->ForEachDestination( func::CapitalizeWords( &capitalizer ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_CapitalizeOptions( void )
{
	CCapitalizeOptionsDialog dialog( this );
	dialog.DoModal();
	GotoDlgCtrl( GetDlgItem( IDC_CAPITALIZE_BUTTON ) );
}

void CMainRenameDialog::OnBnClicked_ChangeCase( void )
{
	m_pFileData->ForEachDestination( func::MakeCase( m_changeCaseButton.GetSelEnum< ChangeCase >() ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_ReplaceDestFiles( void )
{
	CReplaceDialog dlg( this );
	dlg.Execute();
}

void CMainRenameDialog::OnBnClicked_ReplaceAllDelimitersDestFiles( void )
{
	std::tstring delimiterSet = m_delimiterSetCombo.GetCurrentText();
	if ( delimiterSet.empty() )
	{
		AfxMessageBox( str::Format( IDS_NO_DELIMITER_SET ).c_str() );
		ui::TakeFocus( m_delimiterSetCombo );
		return;
	}

	m_pFileData->ForEachDestination( func::ReplaceDelimiterSet( delimiterSet, ui::GetWindowText( &m_newDelimiterEdit ) ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_Undo( void )
{
	ASSERT( m_mode != UndoRollbackMode && m_pFileData->CanUndo() );

	GotoDlgCtrl( GetDlgItem( IDOK ) );
	m_pFileData->RetrieveUndoInfo();
	SwitchMode( UndoRollbackMode );
	SetupFileListView();
}

void CMainRenameDialog::OnFieldChanged( void )
{
	if ( m_mode != Uninit )
		SwitchMode( MakeMode );
}

void CMainRenameDialog::OnPickFormatToken( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU_MENU, popup::FormatPicker );

	m_formatToolbar.TrackButtonMenu( ID_PICK_FORMAT_TOKEN, this, &popupMenu, ui::DropDown );
}

void CMainRenameDialog::OnPickDirPath( void )
{
	// single command or pick menu
	UINT singleCmdId;
	CMenu popupMenu;
	CFileSetUi fileSetUi( m_pFileData );

	if ( fileSetUi.MakePickDirPathMenu( &singleCmdId, &popupMenu ) )
		if ( singleCmdId != 0 )
			OnDirPathPicked( singleCmdId );
		else
		{
			ASSERT_PTR( popupMenu.GetSafeHmenu() );
			m_formatToolbar.TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CMainRenameDialog::OnDirPathPicked( UINT cmdId )
{
	CFileSetUi fileSetUi( m_pFileData );
	std::tstring selDir = fileSetUi.GetPickedDirectory( cmdId );
	if ( CEdit* pComboEdit = (CEdit*)m_formatCombo.GetWindow( GW_CHILD ) )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( selDir.c_str(), TRUE );
	}
}

void CMainRenameDialog::OnPickTextTools( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU_MENU, popup::TextTools );

	m_formatToolbar.TrackButtonMenu( ID_PICK_TEXT_TOOLS, this, &popupMenu, ui::DropDown );
}

void CMainRenameDialog::OnFormatTextToolPicked( UINT cmdId )
{
	GotoDlgCtrl( &m_formatCombo );
	std::tstring oldFormat = m_formatCombo.GetCurrentText();
	std::tstring format = CFileSetUi::ApplyTextTool( cmdId, oldFormat );

	if ( format != oldFormat )
		ui::SetComboEditText( m_formatCombo, format, str::Case );
}

void CMainRenameDialog::OnToggleAutoGenerate( void )
{
	m_autoGenerate = !m_autoGenerate;
	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CMainRenameDialog::OnUpdateAutoGenerate( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_mode != UndoRollbackMode );
	pCmdUI->SetCheck( m_autoGenerate );
}

void CMainRenameDialog::OnNumericSequence( UINT cmdId )
{
	const TCHAR* pInsertFmt = NULL;

	switch ( cmdId )
	{
		case ID_NUMERIC_SEQUENCE_2DIGITS:	pInsertFmt = _T("##"); break;
		case ID_NUMERIC_SEQUENCE_3DIGITS:	pInsertFmt = _T("###"); break;
		case ID_NUMERIC_SEQUENCE_4DIGITS:	pInsertFmt = _T("####"); break;
		case ID_NUMERIC_SEQUENCE_5DIGITS:	pInsertFmt = _T("#####"); break;
		case ID_NUMERIC_SEQUENCE:			pInsertFmt = _T("#"); break;
		case ID_WILDCARD:					pInsertFmt = _T("*"); break;
		case ID_EXTENSION_SEPARATOR:		pInsertFmt = _T("."); break;
		default: ASSERT( false ); return;
	}

	CEdit* pComboEdit = (CEdit*)m_formatCombo.GetWindow( GW_CHILD );
	pComboEdit->ReplaceSel( pInsertFmt, TRUE );
}

void CMainRenameDialog::OnEnChange_SeqCounter( void )
{
	if ( m_autoGenerate )
		AutoGenerateFiles();
	else if ( m_mode != Uninit )
		SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PickRenameActions( void )
{
	m_pickRenameActionsStatic.TrackMenu( this, IDR_CONTEXT_MENU_MENU, popup::MoreRenameActions );
}

void CMainRenameDialog::OnSingleWhitespace( void )
{
	m_pFileData->ForEachDestination( func::SingleWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnRemoveWhitespace( void )
{
	m_pFileData->ForEachDestination( func::RemoveWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnDashToSpace( void )
{
	m_pFileData->ForEachDestination( func::ReplaceDelimiterSet( _T("-"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToDash( void )
{
	m_pFileData->ForEachDestination( func::ReplaceDelimiterSet( _T(" "), _T("-") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnUnderbarToSpace( void )
{
	m_pFileData->ForEachDestination( func::ReplaceDelimiterSet( _T("_"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToUnderbar( void )
{
	m_pFileData->ForEachDestination( func::ReplaceDelimiterSet( _T(" "), _T("_") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnEnsureUniformNumPadding( void )
{
	m_pFileData->EnsureUniformNumPadding();
	PostMakeDest();
}

void CMainRenameDialog::OnCustomDrawFileRenameList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pCustomDraw = (NMLVCUSTOMDRAW*)pNmHdr;

	*pResult = CDRF_DODEFAULT;

	static const CRect emptyRect( 0, 0, 0, 0 );
	if ( emptyRect == pCustomDraw->nmcd.rc )
		return;			// IMP: avoid custom drawing for tooltips

	// scope these variables so that are visible in the debugger
	const CDisplayItem* pItem = reinterpret_cast< const CDisplayItem* >( pCustomDraw->nmcd.lItemlParam );

	switch ( pCustomDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEM | CDDS_ITEMPREPAINT:
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			if ( ListItem_FillBkgnd( pCustomDraw ) )
				*pResult |= CDRF_NEWFONT;
			break;
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			if ( Source == pCustomDraw->iSubItem )
			{
				if ( !useDefaultDraw || pItem->HasMisMatch() )
				{
					//*pResult |= CDRF_NOTIFYPOSTPAINT;		custom draw now, skip default text drawing & post-paint stage
					*pResult |= CDRF_SKIPDEFAULT;
					ListItem_DrawTextDiffs( pCustomDraw );
				}
			}
			else if ( Destination == pCustomDraw->iSubItem )					// && !pItem->m_destFnameExt.empty()
				if ( useDefaultDraw && str::MatchEqual == pItem->m_match )
				{
					pCustomDraw->clrText = GetSysColor( COLOR_GRAYTEXT );		// gray-out text of unmodified dest files
					*pResult |= CDRF_NEWFONT;
				}
				else
				{
					//*pResult |= CDRF_NOTIFYPOSTPAINT;		custom draw now, skip default text drawing & post-paint stage
					*pResult |= CDRF_SKIPDEFAULT;
					ListItem_DrawTextDiffs( pCustomDraw );
				}
			break;
		case CDDS_SUBITEM | CDDS_ITEMPOSTPAINT:		// doesn't get called when using CDRF_SKIPDEFAULT in CDDS_ITEMPREPAINT draw stage
			//ListItem_DrawTextDiffs( pCustomDraw );
			break;
	}
}


// CMainRenameDialog::CDisplayItem implementation

void CMainRenameDialog::CDisplayItem::ComputeMatchSeq( void )
{
	m_srcMatchSeq.clear();
	m_destMatchSeq.clear();
	if ( m_destFnameExt.empty() || str::MatchEqual == m_match )
		return;

	lcs::Comparator< TCHAR, path::GetMatch > comparator( m_srcFnameExt.c_str(), m_srcFnameExt.size(), m_destFnameExt.c_str(), m_destFnameExt.size() );

	std::vector< lcs::CResult< TCHAR > > lcsSeq;
	comparator.Process( lcsSeq );

	m_srcMatchSeq.reserve( m_srcFnameExt.size() );
	m_destMatchSeq.reserve( m_destFnameExt.size() );

	for ( std::vector< lcs::CResult< TCHAR > >::const_iterator it = lcsSeq.begin(); it != lcsSeq.end(); ++it )
		switch ( it->m_matchType )
		{
			case lcs::Equal:
				m_srcMatchSeq.push_back( str::MatchEqual );
				m_destMatchSeq.push_back( str::MatchEqual );
				break;
			case lcs::EqualDiffCase:
				m_srcMatchSeq.push_back( str::MatchEqualDiffCase );
				m_destMatchSeq.push_back( str::MatchEqualDiffCase );
				break;
			case lcs::Insert:
				m_destMatchSeq.push_back( str::MatchNotEqual );
				// discard SRC
				break;
			case lcs::Remove:
				m_srcMatchSeq.push_back( str::MatchNotEqual );
				// discard DEST
				break;
		}

	ASSERT( m_destFnameExt.size() == m_destMatchSeq.size() );
}
