
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
#include "utl/vector_map.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("TouchDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FILE_TOUCH_LIST, Size },
		{ IDC_SHOW_SOURCE_INFO_CHECK, MoveX },

		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },
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
	, m_anyChanges( false )
{
	ASSERT_PTR( m_pFileData );
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetTextEffectCallback( this );

	static const TCHAR s_mixedFormat[] = _T("'(mixed)'");
	m_dateTimeCtrls[ app::ModifiedDate ].SetNullFormat( s_mixedFormat );
	m_dateTimeCtrls[ app::CreatedDate ].SetNullFormat( s_mixedFormat );
	m_dateTimeCtrls[ app::AccessedDate ].SetNullFormat( s_mixedFormat );

	Construct();
}

CTouchFilesDialog::~CTouchFilesDialog()
{
	utl::ClearOwningContainer( m_displayItems );
}

void CTouchFilesDialog::Construct( void )
{
	fs::TFileStatePairMap& rTouchPairs = m_pFileData->GetTouchPairs();
	m_displayItems.reserve( rTouchPairs.size() );
	for ( fs::TFileStatePairMap::iterator itPair = rTouchPairs.begin(); itPair != rTouchPairs.end(); ++itPair )
		m_displayItems.push_back( new CTouchItem( &*itPair, m_pathFormat ) );

	m_dateTimeStates.reserve( app::_DateTimeFieldCount );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_MODIFIED_DATE, app::ModifiedDate ) );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_CREATED_DATE, app::CreatedDate ) );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_ACCESSED_DATE, app::AccessedDate ) );

	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_READONLY_CHECK, CFile::readOnly ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_HIDDEN_CHECK, CFile::hidden ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_SYSTEM_CHECK , CFile::system ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_ARCHIVE_CHECK, CFile::volume ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_DIRECTORY_CHECK, CFile::directory ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_VOLUME_CHECK, CFile::archive ) );
}

void CTouchFilesDialog::SwitchMode( Mode mode )
{
	static const CEnumTags modeTags( _T("&Touch|&Rollback") );

	m_mode = mode;
	ASSERT( m_mode != Uninit );
	ui::SetDlgItemText( m_hWnd, IDOK, modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_COPY_SOURCE_PATHS_BUTTON, IDC_PASTE_FILES_BUTTON, IDC_CLEAR_FILES_BUTTON,
		IDC_MODIFIED_DATE, IDC_CREATED_DATE, IDC_ACCESSED_DATE,
		IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_HIDDEN_CHECK, IDC_ATTRIB_SYSTEM_CHECK, IDC_ATTRIB_ARCHIVE_CHECK
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), m_mode != UndoRollbackMode );
	ui::EnableControl( *this, IDC_UNDO_BUTTON, m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::TouchFiles ) );

	m_anyChanges = utl::Any( m_displayItems, std::mem_fun( &CTouchItem::IsModified ) );
	ui::EnableControl( *this, IDOK, m_mode != TouchMode || m_anyChanges );
}

void CTouchFilesDialog::PostMakeDest( bool silent /*= false*/ )
{
	m_pBatchTransaction.reset();

	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	SetupFileListView();							// fill in and select the found files list
	UpdateFieldControls();
	SwitchMode( TouchMode );
}

bool CTouchFilesDialog::TouchFiles( void )
{
	m_pBatchTransaction.reset( new fs::CBatchTouch( m_pFileData->GetTouchPairs(), this ) );
	return m_pBatchTransaction->TouchFiles();
}

void CTouchFilesDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();

		for ( unsigned int pos = 0; pos != m_displayItems.size(); ++pos )
		{
			CTouchItem* pTouchItem = m_displayItems[ pos ];
			m_fileListCtrl.InsertObjectItem( pos, pTouchItem );		// PathName

			m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( pTouchItem->GetDestState().m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_accessTime ) );

			m_fileListCtrl.SetSubItemText( pos, SrcAttributes, fmt::FormatFileAttributes( pTouchItem->GetSrcState().m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, SrcModifyTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, SrcCreationTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, SrcAccessTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_accessTime ) );
		}
	}

	if ( orgSel != -1 )		// restore selection?
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}
}

void CTouchFilesDialog::UpdateFileListViewDest( void )
{
	REQUIRE( m_displayItems.size() == (size_t)m_fileListCtrl.GetItemCount() );
	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		for ( int pos = 0, count = m_fileListCtrl.GetItemCount(); pos != count; ++pos )
		{
			CTouchItem* pTouchItem = m_displayItems[ pos ];

			m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( pTouchItem->GetDestState().m_attributes ) );
			m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_modifTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_creationTime ) );
			m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_accessTime ) );
		}
	}
}

void CTouchFilesDialog::AccumulateCommonStates( void )
{
	ASSERT( !m_displayItems.empty() );

	// accumulate common values among multiple items, starting with an invalid value, i.e. uninitialized
	multi::SetInvalidAll( m_dateTimeStates );
	multi::SetInvalidAll( m_attribCheckStates );

	for ( size_t i = 0; i != m_displayItems.size(); ++i )
		AccumulateItemStates( m_displayItems[ i ] );
}

void CTouchFilesDialog::AccumulateItemStates( const CTouchItem* pTouchItem )
{
	ASSERT_PTR( pTouchItem );

	m_dateTimeStates[ app::ModifiedDate ].Accumulate( pTouchItem->GetDestState().m_modifTime );
	m_dateTimeStates[ app::CreatedDate ].Accumulate( pTouchItem->GetDestState().m_creationTime );
	m_dateTimeStates[ app::AccessedDate ].Accumulate( pTouchItem->GetDestState().m_accessTime );

	for ( std::vector< multi::CAttribCheckState >::iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->Accumulate( pTouchItem->GetDestState().m_attributes );
}

void CTouchFilesDialog::UpdateFieldControls( void )
{
	// commit common values to field controls
	for ( std::vector< multi::CDateTimeState >::const_iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
		itDateTimeState->UpdateCtrl( this );

	for ( std::vector< multi::CAttribCheckState >::const_iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->UpdateCtrl( this );
}

void CTouchFilesDialog::UpdateFieldsFromSel( int selIndex )
{
	if ( selIndex != -1 )
	{
		multi::SetInvalidAll( m_dateTimeStates );
		multi::SetInvalidAll( m_attribCheckStates );

		AccumulateItemStates( m_fileListCtrl.GetPtrAt< CTouchItem >( selIndex ) );
	}
	else
	{
		multi::ClearAll( m_dateTimeStates );
		multi::ClearAll( m_attribCheckStates );
	}

	UpdateFieldControls();
}

void CTouchFilesDialog::InputFields( void )
{
	for ( std::vector< multi::CDateTimeState >::iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
		itDateTimeState->InputCtrl( this );

	for ( std::vector< multi::CAttribCheckState >::iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->InputCtrl( this );
}

void CTouchFilesDialog::ApplyFields( void )
{
	// apply valid edits, i.e. if not null
	for ( size_t i = 0; i != m_displayItems.size(); ++i )
	{
		CTouchItem* pTouchItem = m_displayItems[ i ];

		for ( std::vector< multi::CDateTimeState >::const_iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
			itDateTimeState->Apply( pTouchItem );

		for ( std::vector< multi::CAttribCheckState >::const_iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
			itAttribState->Apply( pTouchItem );
	}
}

bool CTouchFilesDialog::VisibleAllSrcColumns( void ) const
{
	for ( int column = SrcAttributes; column <= SrcAccessTime; ++column )
		if ( 0 == m_fileListCtrl.GetColumnWidth( column ) )
			return false;

	return true;
}

int CTouchFilesDialog::FindItemPos( const fs::CPath& keyPath ) const
{
	return static_cast< int >( utl::BinaryFindPos( m_displayItems, keyPath, CTouchItem::ToKeyPath() ) );
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
	static const ui::CTextEffect modPathName( ui::Bold ), modDest( ui::Bold, app::ColorModifiedText ), modSrc( ui::Regular, app::ColorDeletedText ), errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CTouchItem* pTouchItem = checked_static_cast< const CTouchItem* >( pSubject );
	const ui::CTextEffect* pTextEffect = NULL;
	bool isModified = false, isSrc = false;

	switch ( subItem )
	{
		case PathName:
			isModified = pTouchItem->IsModified();
			if ( isModified )
				pTextEffect = &modPathName;
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

BOOL CTouchFilesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		CBaseMainDialog::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CTouchFilesDialog::DoDataExchange( CDataExchange* pDX )
{
	ASSERT( COUNT_OF( m_dateTimeCtrls ) == m_dateTimeStates.size() );

	DDX_Control( pDX, IDC_FILE_TOUCH_LIST, m_fileListCtrl );

	for ( size_t i = 0; i != m_dateTimeStates.size(); ++i )
		DDX_Control( pDX, m_dateTimeStates[ i ].m_ctrlId, m_dateTimeCtrls[ i ] );

	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS );
	ui::DDX_ButtonIcon( pDX, IDC_UNDO_BUTTON, ID_EDIT_UNDO );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( Uninit == m_mode )
		{
			AccumulateCommonStates();

			SetupFileListView();			// fill in and select the found files list
			UpdateFieldControls();
			CheckDlgButton( IDC_SHOW_SOURCE_INFO_CHECK, VisibleAllSrcColumns() );

			SwitchMode( ApplyFieldsMode );
		}
	}

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTouchFilesDialog, CBaseMainDialog )
	ON_BN_CLICKED( IDC_UNDO_BUTTON, OnBnClicked_Undo )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestStates )
	ON_BN_CLICKED( IDC_CLEAR_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_BN_CLICKED( IDC_SHOW_SOURCE_INFO_CHECK, OnBnClicked_ShowSrcColumns )
	ON_COMMAND_RANGE( IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_VOLUME_CHECK, OnToggle_Attribute )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FILE_TOUCH_LIST, OnLvnItemChanged_TouchList )
	ON_NOTIFY_RANGE( DTN_DATETIMECHANGE, IDC_MODIFIED_DATE, IDC_ACCESSED_DATE, OnDtnDateTimeChange_DateTimeCtrl )
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
		case ApplyFieldsMode:
			InputFields();
			ApplyFields();
			PostMakeDest();
			break;
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

void CTouchFilesDialog::OnFieldChanged( void )
{
	if ( m_mode != Uninit )
		SwitchMode( ApplyFieldsMode );
}

void CTouchFilesDialog::OnBnClicked_Undo( void )
{
	ASSERT( m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::TouchFiles ) );

	GotoDlgCtrl( GetDlgItem( IDOK ) );
	m_pFileData->RetrieveUndoInfo( app::TouchFiles );
	SwitchMode( UndoRollbackMode );
	SetupFileListView();
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
	PostMakeDest();
}

void CTouchFilesDialog::OnBnClicked_ShowSrcColumns( void )
{
	if ( VisibleAllSrcColumns() )
	{
		for ( int column = SrcAttributes; column <= SrcAccessTime; ++column )
			m_fileListCtrl.SetColumnWidth( column, 0 );		// hide column
	}
	else
		m_fileListCtrl.ResetColumnLayout();					// show all columns with default layout
}

void CTouchFilesDialog::OnToggle_Attribute( UINT checkId )
{
	if ( multi::CAttribCheckState* pAttribState = multi::FindWithCtrlId( m_attribCheckStates, checkId ) )
	{
		UINT checkState = pAttribState->GetChecked( this );
		if ( checkState != BST_INDETERMINATE )
		{
			OnFieldChanged();
			m_fileListCtrl.Invalidate();
		}
	}
}

void CTouchFilesDialog::OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_LISTVIEW* pNmList = (NM_LISTVIEW*)pNmHdr;
	if ( CReportListControl::IsSelectionChangedNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		UpdateFieldsFromSel( m_fileListCtrl.GetCurSel() );

	*pResult = 0;
}

void CTouchFilesDialog::OnDtnDateTimeChange_DateTimeCtrl( UINT dtId, NMHDR* pNmHdr, LRESULT* pResult )
{
	NMDATETIMECHANGE* pChange = (NMDATETIMECHANGE*)pNmHdr;
	pChange, dtId;
	*pResult = 0L;

	OnFieldChanged();
}
