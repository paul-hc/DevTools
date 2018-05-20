
#include "stdafx.h"
#include "TouchFilesDialog.h"
#include "TouchItem.h"
#include "FileWorkingSet.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"
#include "utl/TimeUtl.h"
#include "utl/resource.h"


//enum CustomColors { ColorDeletedText = color::Red, ColorModifiedText = color::Blue, ColorErrorBk = color::PastelPink };
static const bool TO_DO = false;


namespace reg
{
	static const TCHAR section_dialog[] = _T("TouchDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FORMAT_COMBO, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_FILE_TOUCH_LIST, Size },

		{ IDC_SOURCE_FILES_GROUP, MoveY },
		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },

		{ IDC_DESTINATION_FILES_GROUP, Move },
		{ IDC_PASTE_FILES_BUTTON, Move },
		{ IDC_CLEAR_FILES_BUTTON, Move },

		{ IDOK, MoveX },
		{ IDC_UNDO_BUTTON, MoveX },
		{ IDCANCEL, MoveX }
	};
}


CTouchFilesDialog::CTouchFilesDialog( CFileWorkingSet* pFileData, CWnd* pParent )
	: CBaseMainDialog( IDD_TOUCH_FILES_DIALOG, pParent )
	, m_pFileData( pFileData )
	, m_mode( Uninit )
	, m_fileListCtrl( IDC_FILE_TOUCH_LIST, LVS_EX_GRIDLINES | CReportListControl::DefaultStyleEx )
{
	ASSERT_PTR( m_pFileData );
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
}

CTouchFilesDialog::~CTouchFilesDialog()
{
	utl::ClearOwningContainer( m_displayItems );
}

void CTouchFilesDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		//CScopedListTextSelection selection( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();
		utl::ClearOwningContainer( m_displayItems );

		fs::TFileStatePairMap& rTouchPairs = m_pFileData->GetTouchPairs();
		m_displayItems.reserve( rTouchPairs.size() );
		bool hasMultipleDirPaths = utl::HasMultipleDirPaths( rTouchPairs );

		int pos = 0;
		for ( fs::TFileStatePairMap::iterator itPair = rTouchPairs.begin(); itPair != rTouchPairs.end(); ++itPair, ++pos )
		{
			CTouchItem* pItem = new CTouchItem( &*itPair, hasMultipleDirPaths );
			m_displayItems.push_back( pItem );
			m_fileListCtrl.InsertObjectItem( pos, pItem );		// Filename

			m_fileListCtrl.SetSubItemText( pos, DestAttributes, num::FormatHexNumber( itPair->second.m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( itPair->second.m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( itPair->second.m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( itPair->second.m_accessTime ) );

			m_fileListCtrl.SetSubItemText( pos, SrcAttributes, num::FormatHexNumber( itPair->first.m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, SrcModifyTime, time_utl::FormatTimestamp( itPair->first.m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, SrcCreationTime, time_utl::FormatTimestamp( itPair->first.m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, SrcAccessTime, time_utl::FormatTimestamp( itPair->first.m_accessTime ) );
		}
	}

	if ( orgSel != -1 )		// restore selection?
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}

m_fileListCtrl.MarkCellAt( 1, Filename, ui::CTextEffect( ui::Bold ) );
m_fileListCtrl.MarkCellAt( 2, SrcModifyTime, ui::CTextEffect( ui::Bold | ui::Italic | ui::Underline ) );
m_fileListCtrl.MarkCellAt( 3, CReportListControl::EntireRecord, ui::CTextEffect( ui::Italic | ui::Underline, color::Red, color::LightGreenish ) );
m_fileListCtrl.MarkCellAt( 3, Filename, ui::CTextEffect( ui::Bold, color::Red ) );
m_fileListCtrl.MarkCellAt( 3, SrcCreationTime, ui::CTextEffect( ui::Bold, color::Blue ) );

m_fileListCtrl.MarkCellAt( 5, CReportListControl::EntireRecord, ui::CTextEffect( ui::Underline, color::Green, color::PastelPink ) );
m_fileListCtrl.MarkCellAt( 5, SrcCreationTime, ui::CTextEffect() );

m_fileListCtrl.Invalidate();
}

int CTouchFilesDialog::FindItemPos( const fs::CPath& keyPath ) const
{
	return static_cast< int >( utl::BinaryFindPos( m_displayItems, keyPath, CTouchItem::ToKeyPath() ) );
}

void CTouchFilesDialog::SwitchMode( Mode mode )
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
	ui::EnableControl( *this, IDC_UNDO_BUTTON, m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::TouchFiles ) );
}

void CTouchFilesDialog::PostMakeDest( bool silent /*= false*/ )
{
	m_pBatchTransaction.reset();

	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	SetupFileListView();							// fill in and select the found files list
	SwitchMode( TouchMode );
}

CWnd* CTouchFilesDialog::GetWnd( void )
{
	return this;
}

CLogger* CTouchFilesDialog::GetLogger( void )
{
	return &app::GetLogger();
}

fs::UserFeedback CTouchFilesDialog::HandleFileError( const fs::CPath& sourcePath, const std::tstring& message )
{
	int pos = FindItemPos( sourcePath );
	if ( pos != -1 )
		m_fileListCtrl.EnsureVisible( pos, FALSE );
	m_fileListCtrl.Invalidate();

	switch ( AfxMessageBox( message.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
	{
		default: ASSERT( false );
		case IDIGNORE:	return fs::Ignore;
		case IDRETRY:	return fs::Retry;
		case IDABORT:	return fs::Abort;
	}
}

bool CTouchFilesDialog::TouchFiles( void )
{
	m_pBatchTransaction.reset( new fs::CBatchTouch( m_pFileData->GetTouchPairs(), this ) );
	return m_pBatchTransaction->TouchFiles();
}

void CTouchFilesDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_FILE_TOUCH_LIST, m_fileListCtrl );

	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS );
	ui::DDX_ButtonIcon( pDX, IDC_UNDO_BUTTON, ID_EDIT_UNDO );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( Uninit == m_mode )
		{
			SetupFileListView();			// fill in and select the found files list
			SwitchMode( TouchMode );
		}
	}

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTouchFilesDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED( IDC_UNDO_BUTTON, OnBnClicked_Undo )

//	ON_NOTIFY( NM_CUSTOMDRAW, IDC_FILE_TOUCH_LIST, OnNmCustomDraw_FileList )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestStates )
	ON_BN_CLICKED( IDC_CLEAR_FILES_BUTTON, OnBnClicked_ResetDestFiles )
END_MESSAGE_MAP()

BOOL CTouchFilesDialog::OnInitDialog( void )
{
	BOOL defaultFocus = CBaseMainDialog::OnInitDialog();
	return defaultFocus;
}

void CTouchFilesDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case TouchMode:
			if ( TouchFiles() )
			{
				m_pFileData->SaveUndoInfo( app::TouchFiles, m_pBatchTransaction->GetCommittedKeys() );
				CBaseMainDialog::OnOK();
			}
			else
			{
				m_pFileData->SaveUndoInfo( app::TouchFiles, m_pBatchTransaction->GetCommittedKeys() );
				SwitchMode( TouchMode );
			}
			break;
		case UndoRollbackMode:
			if ( TouchFiles() )
			{
				m_pFileData->CommitUndoInfo( app::TouchFiles );
				CBaseMainDialog::OnOK();
			}
			else
				SwitchMode( TouchMode );
			break;
	}
}

void CTouchFilesDialog::OnDestroy( void )
{
//	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_autoGenerate, m_autoGenerate );

	CBaseMainDialog::OnDestroy();
}

void CTouchFilesDialog::OnBnClicked_CopySourceFiles( void )
{
	ASSERT( TO_DO );
//	if ( !m_pFileData->CopyClipSourcePaths( FilenameExt, this ) )
//		AfxMessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CTouchFilesDialog::OnBnClicked_ResetDestFiles( void )
{
m_fileListCtrl.Invalidate();
return;
	ASSERT( TO_DO );
	m_pFileData->ClearDestinations();

	SetupFileListView();	// fill in and select the found files list
	SwitchMode( TouchMode );
}

void CTouchFilesDialog::OnBnClicked_PasteDestStates( void )
{
	ASSERT( TO_DO );
	try
	{
//		m_pFileData->PasteClipDestinationPaths( this );
		PostMakeDest();
	}
	catch ( CRuntimeException& e )
	{
		e.ReportError();
	}
}

void CTouchFilesDialog::OnBnClicked_Undo( void )
{
	ASSERT( m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::TouchFiles ) );

	GotoDlgCtrl( GetDlgItem( IDOK ) );
	m_pFileData->RetrieveUndoInfo( app::TouchFiles );
	SwitchMode( UndoRollbackMode );
	SetupFileListView();
}

void CTouchFilesDialog::OnFieldChanged( void )
{
	if ( m_mode != Uninit )
		SwitchMode( TouchMode );
}

void CTouchFilesDialog::OnNmCustomDraw_FileList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pCustomDraw = (NMLVCUSTOMDRAW*)pNmHdr;

	*pResult = CDRF_DODEFAULT;

	pCustomDraw;

/*	static const CRect emptyRect( 0, 0, 0, 0 );
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
	}*/
}
