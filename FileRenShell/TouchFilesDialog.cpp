
#include "stdafx.h"
#include "TouchFilesDialog.h"
#include "TouchItem.h"
#include "FileWorkingSet.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/FmtUtils.h"
#include "utl/EnumTags.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"
#include "utl/TimeUtl.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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

		{ IDC_DESTINATION_FILES_GROUP, MoveY },
		{ IDC_PASTE_FILES_BUTTON, MoveY },
		{ IDC_CLEAR_FILES_BUTTON, MoveY },

		{ IDOK, MoveX },
		{ IDC_UNDO_BUTTON, MoveX },
		{ IDCANCEL, MoveX }
	};
}


CTouchFilesDialog::CTouchFilesDialog( CFileWorkingSet* pFileData, CWnd* pParent )
	: CBaseMainDialog( IDD_TOUCH_FILES_DIALOG, pParent )
	, m_pFileData( pFileData )
	, m_pathFormat( m_pFileData->HasMixedDirPaths() ? fmt::FullPath : fmt::FilenameExt )
	, m_mode( Uninit )
	, m_fileListCtrl( IDC_FILE_TOUCH_LIST, LVS_EX_GRIDLINES | CReportListControl::DefaultStyleEx )
{
	ASSERT_PTR( m_pFileData );
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );

	m_fileListCtrl.SetTextEffectCallback( this );
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
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();
		utl::ClearOwningContainer( m_displayItems );

		fs::TFileStatePairMap& rTouchPairs = m_pFileData->GetTouchPairs();
		m_displayItems.reserve( rTouchPairs.size() );

		int pos = 0;
		for ( fs::TFileStatePairMap::iterator itPair = rTouchPairs.begin(); itPair != rTouchPairs.end(); ++itPair, ++pos )
		{
			CTouchItem* pItem = new CTouchItem( &*itPair, m_pathFormat );
			m_displayItems.push_back( pItem );
			m_fileListCtrl.InsertObjectItem( pos, pItem );		// Filename

			m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( itPair->second.m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( itPair->second.m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( itPair->second.m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( itPair->second.m_accessTime ) );

			m_fileListCtrl.SetSubItemText( pos, SrcAttributes, fmt::FormatFileAttributes( itPair->first.m_attributes ) );
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
}

int CTouchFilesDialog::FindItemPos( const fs::CPath& keyPath ) const
{
	return static_cast< int >( utl::BinaryFindPos( m_displayItems, keyPath, CTouchItem::ToKeyPath() ) );
}

void CTouchFilesDialog::SwitchMode( Mode mode )
{
	static const CEnumTags modeTags( _T("&Touch|&Rollback") );

	m_mode = mode;
	ASSERT( m_mode != Uninit );
	ui::SetWindowText( ::GetDlgItem( m_hWnd, IDOK ), modeTags.FormatUi( m_mode ) );

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

void CTouchFilesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, utl::ISubject* pSubject, int subItem ) const
{
	static const ui::CTextEffect modFilename( ui::Bold ), modDest( ui::Bold, app::ColorModifiedText ), modSrc( ui::Regular, app::ColorDeletedText ), errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CTouchItem* pTouchItem = checked_static_cast< const CTouchItem* >( pSubject );
	const ui::CTextEffect* pTextEffect = NULL;
	bool isModified = false, isSrc = false;

	switch ( subItem )
	{
		case Filename:
			isModified = pTouchItem->IsModified();
			if ( isModified )
				pTextEffect = &modFilename;
			break;
		case SrcAttributes:
			isSrc = true;		// fall-through
		case DestAttributes:
			isModified = pTouchItem->GetDestState().m_attributes != pTouchItem->GetSrcState().m_attributes;
			break;
		case SrcModifyTime:
			isSrc = true;		// fall-through
		case DestModifyTime:
			isModified = pTouchItem->GetDestState().m_modifTime != pTouchItem->GetSrcState().m_modifTime;
			break;
		case SrcCreationTime:
			isSrc = true;		// fall-through
		case DestCreationTime:
			isModified = pTouchItem->GetDestState().m_creationTime != pTouchItem->GetSrcState().m_creationTime;
			break;
		case SrcAccessTime:
			isSrc = true;		// fall-through
		case DestAccessTime:
			isModified = pTouchItem->GetDestState().m_accessTime != pTouchItem->GetSrcState().m_accessTime;
			break;
	}

	if ( !isSrc && m_pBatchTransaction.get() != NULL )
		if ( m_pBatchTransaction->ContainsError( pTouchItem->GetKeyPath() ) )
			rTextEffect |= errorBk;

	if ( pTextEffect != NULL )
		rTextEffect |= *pTextEffect;
	else if ( isModified )
		rTextEffect |= isSrc ? modSrc : modDest;
}

bool CTouchFilesDialog::TouchFiles( void )
{
	m_pBatchTransaction.reset( new fs::CBatchTouch( m_pFileData->GetTouchPairs(), this ) );
	return m_pBatchTransaction->TouchFiles();
}

BOOL CTouchFilesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		CBaseMainDialog::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
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
	if ( !m_pFileData->CopyClipSourceFileStates( this ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CTouchFilesDialog::OnBnClicked_PasteDestStates( void )
{
	try
	{
		m_pFileData->PasteClipDestinationFileStates( this );
		PostMakeDest();
	}
	catch ( CRuntimeException& e )
	{
		e.ReportError();
	}
}

void CTouchFilesDialog::OnBnClicked_ResetDestFiles( void )
{
	m_pFileData->ResetDestinations();

	SetupFileListView();	// fill in and select the found files list
	SwitchMode( TouchMode );
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
