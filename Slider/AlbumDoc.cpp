
#include "stdafx.h"
#include "AlbumDoc.h"
#include "Workspace.h"
#include "DocTemplates.h"
#include "AlbumSettingsDialog.h"
#include "ArchiveImagesDialog.h"
#include "DefinePasswordDialog.h"
#include "AlbumImageView.h"
#include "AlbumThumbListView.h"
#include "ImageArchiveStg.h"
#include "FileOperation.h"
#include "Application.h"
#include "resource.h"
#include "utl/Serialization.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumDoc, CDocumentBase )


CAlbumDoc::CAlbumDoc( void )
	: CDocumentBase()
	, m_fileModelSchema( app::Slider_LatestModelSchema )
	, m_bkColor( CLR_DEFAULT )
	, m_docFlags( 0 )
{
}

CAlbumDoc::~CAlbumDoc()
{
	// at exit, close the image storages associated with this document (if any)
	m_fileList.CloseAssocImageArchiveStgs();
}

BOOL CAlbumDoc::OnNewDocument( void )
{
	return CDocumentBase::OnNewDocument();
}

void CAlbumDoc::Serialize( CArchive& archive )
{
	REQUIRE( serial::IsFileBasedArchive( archive ) );			// mem-based document serialization not supported/necessary (for now)

	CAlbumImageView* pActiveView = GetOwnActiveAlbumView();
	fs::CPath archiveStgPath = path::ExtractPhysical( archive.m_strFileName.GetString() );

	if ( archive.IsStoring() )
	{
		SetFlag( m_docFlags, PresistImageState, HasFlag( CWorkspace::GetFlags(), wf::PersistAlbumImageState ) );

		if ( pActiveView != NULL )
		{
			// explicitly copy the persistent attributes from active view
			m_slideData = pActiveView->GetSlideData();
			m_bkColor = pActiveView->GetRawBkColor();

			m_pImageState.reset( new CImageState );
			pActiveView->MakeImageState( m_pImageState.get() );
		}

		bool isSaveAs = !path::EquivalentPtr( m_strPathName, archiveStgPath.GetPtr() );

		if ( !isSaveAs && m_fileModelSchema != app::Slider_LatestModelSchema )
		{
			std::tstring message = str::Format( _T("(!) Attempt to save older document to the latest version:\n  %s\n  Old version: %s\n  Latest version: %s"),
				archiveStgPath.GetPtr(),
				app::FormatSliderVersion( m_fileModelSchema ).c_str(),
				app::FormatSliderVersion( app::Slider_LatestModelSchema ).c_str() );

			bool proceed = IDOK == AfxMessageBox( message.c_str(), MB_OKCANCEL | MB_ICONQUESTION );
			if ( !proceed )
				message += _T("\n CANCELLED by user!");

			app::LogLine( message.c_str() );

			if ( proceed )
				m_fileModelSchema = app::Slider_LatestModelSchema;
			else
				throw new mfc::CUserAbortedException;
		}
	}

	// version backwards compatibility hack: check if a valid version is saved as first UINT
	CSlideData::TFirstDataMember firstValue = UINT_MAX;
	std::auto_ptr< serial::CScopedLoadingArchive > pLoadingArchive;

	if ( archive.IsLoading() )
	{
		archive >> firstValue;

		if ( firstValue >= app::Slider_v3_2 && firstValue <= app::Slider_LatestModelSchema )	// valid version saved?
		{	// let model details know the loading archive version
			m_fileModelSchema = static_cast< app::ModelSchema >( firstValue );					// store original document model schema (required for further storage access/metadata lookups)
			firstValue = UINT_MAX;			// mark as extracted (as file ModelSchema)
		}
		else
			m_fileModelSchema = app::Slider_v3_1;												// assume an old backwards-compatible model schema (not saved back in the day)

		pLoadingArchive.reset( new serial::CScopedLoadingArchive( archive, m_fileModelSchema ) );
	}
	else
	{
		m_fileModelSchema = app::Slider_LatestModelSchema;				// always save the latest model schema version as first UINT in the archive
		archive << m_fileModelSchema;
	}

	// backwards compatibility: pass firstValue read
	m_slideData.Stream( archive, firstValue != UINT_MAX ? &firstValue : NULL );

	if ( archive.IsStoring() )
	{
		archive << m_bkColor;
		archive << m_docFlags;
	}
	else
	{
		archive >> m_bkColor;
		archive >> m_docFlags;
	}

	m_fileList.Stream( archive );

	if ( archive.IsLoading() )
		m_fileList.CheckReparentFileAttrs( archiveStgPath.GetPtr(), CFileList::Loading );		// reparent embedded image paths with current doc stg path

	m_dropUndoStack.Stream( archive );
	m_dropRedoStack.Stream( archive );
	InitAutoDropRecipient();

	if ( HasFlag( m_slideData.m_viewFlags, af::SaveCustomOrderUndoRedo ) )
	{
		m_customOrderUndoStack.Stream( archive );
		m_customOrderRedoStack.Stream( archive );
	}

	if ( HasFlag( m_docFlags, PresistImageState ) )
		serial::StreamPtr( archive, m_pImageState );

	if ( archive.IsStoring() )
		SetModifiedFlag( Clean );

	app::LogLine( _T("%s album %s with model schema version %s"),
		archive.IsLoading() ? _T("Loaded") : _T("Saved"),
		archiveStgPath.GetPtr(),
		app::FormatSliderVersion( m_fileModelSchema ).c_str() );
}

CImageState* CAlbumDoc::GetImageState( void ) const
{
	if ( CImageState* pLoadingState = CWorkspace::Instance().GetLoadingImageState() )		// loading workspace saved image states?
		return pLoadingState;					// workspace image states takes precedence, so we'll ignore album's image state

	return m_pImageState.get();
}

const fs::ImagePathKey& CAlbumDoc::GetImageFilePathAt( int index ) const
{
	if ( IsValidIndex( index ) )
		return m_fileList.GetFileAttr( index ).GetPathKey();

	return CWicImage::s_nullKey;
}

void CAlbumDoc::QueryNeighboringPathKeys( std::vector< fs::ImagePathKey >& rNeighbours, size_t index ) const
{
	if ( index > 0 )
		rNeighbours.push_back( m_fileList.GetFileAttr( index - 1 ).GetPathKey() );
	if ( index < GetImageCount() - 1 )
		rNeighbours.push_back( m_fileList.GetFileAttr( index + 1 ).GetPathKey() );
}

// use this with care since it violates Model/View design pattern
CAlbumImageView* CAlbumDoc::GetOwnActiveAlbumView( void ) const
{
	CFrameWnd* pParentFrame = NULL;
	if ( POSITION pos = GetFirstViewPosition() )
		pParentFrame = GetNextView( pos )->GetParentFrame();
	if ( pParentFrame != NULL )
	{
		if ( CView* pActiveView = pParentFrame->GetActiveView() )
			if ( CAlbumImageView* pActiveAlbumView = dynamic_cast< CAlbumImageView* >( pActiveView ) )
				return pActiveAlbumView;
			else if ( CAlbumThumbListView* pActiveThumbView = dynamic_cast< CAlbumThumbListView* >( pActiveView ) )
				return pActiveThumbView->GetAlbumImageView();
	}
	return NULL;
}

CSlideData* CAlbumDoc::GetActiveSlideData( void )
{
	CAlbumImageView* pActiveView = GetOwnActiveAlbumView();
	return pActiveView != NULL ? &pActiveView->RefSlideData() : &m_slideData;
}

bool CAlbumDoc::SaveAsArchiveStg( const fs::CPath& newStgPath )
{
	REQUIRE( app::IsImageArchiveDoc( newStgPath.GetPtr() ) );

	try
	{
		fs::CPath oldDocPath( GetPathName().GetString() );
		bool saveAs = newStgPath != oldDocPath;

		if ( saveAs )
		{
			bool straightStgFileCopy = false;

			if ( !IsModified() )												// in synch with the file?
				if ( app::Slider_LatestModelSchema == m_fileModelSchema )		// latest model schema? (will always save with Slider_LatestModelSchema)
					if ( app::IsImageArchiveDoc( oldDocPath.GetPtr() ) )
						if ( m_fileList.HasConsistentDeepStreams() )
							straightStgFileCopy = true;

			if ( straightStgFileCopy )
			{
				CFileOperation fileOp( true );

				fileOp.Copy( fs::ToFlexPath( oldDocPath ), fs::ToFlexPath( newStgPath ) );		// optimization: straight stg file copy
				fs::MakeFileWritable( newStgPath.GetPtr() );									// just in case source was read-only
			}
			else
			{
				CImageArchiveStg::DiscardCachedImages( oldDocPath );

				CArchiveImagesContext archiveContext;
				CFileList tempFileList = m_fileList;			// temp copy so that it can display original thumbnails while creating, avoiding sharing errors

				// don't pass the document, it's too early to save the album stream, since we're working on a tempFileList copy
				if ( archiveContext.CreateArchiveStgFile( &tempFileList, newStgPath, NULL /*this*/ ) )
					m_fileList = tempFileList;					// assign the results
				else
					return false;
			}
		}

		{
			CPushThrowMode pushThrow( &CImageArchiveStg::Factory(), true );
			m_fileList.CheckReparentFileAttrs( newStgPath.GetPtr(), CFileList::Saving );		// reparent with newStgPath before saving the album info

			if ( CImageArchiveStg::Factory().SaveAlbumDoc( this, newStgPath ) )
				if ( CImageArchiveStg* pSavedImageStg = CImageArchiveStg::Factory().FindStorage( newStgPath ) )
					pSavedImageStg->StoreFileModelSchema( m_fileModelSchema );
		}

		return !saveAs || BuildAlbum( newStgPath );				// reload from the new archive document file so that we reinitialize m_fileList
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}
}

bool CAlbumDoc::BuildAlbum( const fs::CPath& filePath )
{
	bool opening = DirtyOpening == IsModified();
	bool loadedStgAlbumStream = false;

	m_slideData.SetCurrentIndex( 0 );				// may get overridden by subsequent load of album doc
	try
	{
		if ( app::IsImageArchiveDoc( filePath.GetPtr() ) )
		{
			if ( CImageArchiveStg::Factory().VerifyPassword( filePath ) )
			{
				loadedStgAlbumStream = CImageArchiveStg::Factory().LoadAlbumDoc( this, filePath );		// album stream is optional for older archives: not an error if missing
				if ( loadedStgAlbumStream )
				{
					CImageArchiveStg* pLoadedImageStg = CImageArchiveStg::Factory().FindStorage( filePath );
					ASSERT_PTR( pLoadedImageStg );
					pLoadedImageStg->StoreFileModelSchema( m_fileModelSchema );
				}
			}
			else
				return false;
		}

		if ( !loadedStgAlbumStream )				// directory path or archive stg missing album stream?
			if ( m_fileList.SetupSearchPath( filePath ) )
				m_fileList.SearchForFiles();
			else
				return false;
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}

	SetModifiedFlag( Clean );

	if ( !opening )					// not too early for view updates?
	{
		OnFileListChanged();		// update the UI

		if ( loadedStgAlbumStream )
			UpdateAllViews( NULL, Hint_DocSlideDataChanged );		// refresh view navigation from document (selected pos, etc)
	}

	return true;		// keep it open regardless /*m_fileList.AnyFoundFiles();*/
}

void CAlbumDoc::RegenerateFileList( FileListChangeType reason /*= FL_Init*/ )
{
	if ( FL_Regeneration == reason )
		UpdateAllViewsOfType< CAlbumImageView >( NULL, Hint_BackupCurrSelection );		// backup current selection for all the owned views before re-generating the m_fileList member

	// We can't rely on reordering information since there might be new or removed files.
	// However, we can keep file copy/move operations in undo/redo buffers.
	ClearCustomOrder( custom_order::CM_ClearReorder );
	try
	{
		CWaitCursor	wait;
		m_fileList.SearchForFiles( false );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
	}
	SetModifiedFlag( Dirty );

	// update UI
	OnFileListChanged( reason );

	if ( FL_Regeneration == reason )
		UpdateAllViewsOfType< CAlbumImageView >( NULL, Hint_RestoreSelection );		// restore backed-up selection for all the views

	if ( FL_AutoDropOp == reason )
		// since auto-drop operation is usually done from opened albums (as well as Windows Explorer),
		// also redraw any of the opened views that may be affected by this, except for those views
		// belonging to this document, which is just updated.
		app::GetApp()->UpdateAllViews( Hint_FileChanged, this );
}

bool CAlbumDoc::EditAlbum( CAlbumImageView* pActiveView )
{
	CAlbumSettingsDialog dlg( m_fileList, pActiveView->GetSlideData().GetCurrentIndex(), pActiveView );
	if ( dlg.DoModal() != IDOK )
		return false;

	m_fileList = dlg.m_fileList;
	m_slideData.SetCurrentIndex( dlg.m_currentIndex );
	// clear auto-drop context since the drop directory may have changed
	ClearAutoDropContext();

	SetModifiedFlag( Dirty );		// mark document as modified in order to prompt for saving

	OnFileListChanged();
	OnAutoDropRecipientChanged();
	return true;
}

void CAlbumDoc::ClearCustomOrder( custom_order::ClearMode clearMode /*= custom_order::CM_ClearAll*/ )
{
	m_customOrderUndoStack.ClearStack( clearMode );
	m_customOrderRedoStack.ClearStack( clearMode );
}

bool CAlbumDoc::MakeCustomOrder( int& rToDestIndex, std::vector< int >& rToMoveIndexes )
{
	custom_order::COpStep step;

	step.m_toMoveIndexes = rToMoveIndexes;
	step.m_toDestIndex = rToDestIndex;

	if ( !m_fileList.MoveCustomOrderIndexes( rToDestIndex, rToMoveIndexes ) )
		return false;

	step.m_newDestIndex = rToDestIndex;
	m_customOrderUndoStack.push_front( step );		// push to the undo stack the current custom order step
	m_customOrderRedoStack.ClearStack();			// after custom order must clear the redo stack
	return true;
}

bool CAlbumDoc::UndoRedoCustomOrder( custom_order::COpStack& rFromStack, custom_order::COpStack& rToStack, bool isUndoOp )
{
	ASSERT( !rFromStack.empty() );					// FROM stack must not be empty

	custom_order::COpStep& topStep = rFromStack.front();

	if ( topStep.IsArchivingOperation() )
	{	// first prompt to undo/redo a file copy/move operation
		std::tstring message = str::Format( IDS_PROMPT_COPYMOVEFILEOP,
			isUndoOp ? _T("undo") : _T("redo"),
			topStep.m_fileOp == FOP_FileCopy ? _T("copy") : _T("move") );

		if ( AfxMessageBox( message.c_str(), MB_OKCANCEL | MB_ICONQUESTION ) != IDOK )
			return false;

		std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > > errorPairs;
		if ( !topStep.m_archivedImages.CanCommitOperations( errorPairs, topStep.m_fileOp, isUndoOp ) )
		{
			message = str::Format( IDS_UNDOREDO_INVALID_FILE_PAIRS,
				isUndoOp ? _T("undo") : _T("redo"),
				errorPairs.size(),
				topStep.m_fileOp == FOP_FileCopy ? _T("copy") : _T("move"),
				topStep.m_archivedImages.GetPathPairs().size() );
			switch ( AfxMessageBox( message.c_str(), MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
			{
				case IDABORT:	rFromStack.pop_front(); return false;		// remove the faulty entry from the stack
				case IDRETRY:	return false;
				case IDIGNORE:	break;
			}
		}

		if ( !topStep.m_archivedImages.CommitOperations( topStep.m_fileOp, isUndoOp ) )
			return false;

		// undo operation from FROM to TO stacks
		rToStack.push_front( rFromStack.front() );		// transfer FROM->TO by value
		rFromStack.pop_front();

		if ( IDOK == AfxMessageBox( IDS_PROMPT_REGENERATE_ALBUM, MB_OKCANCEL | MB_ICONQUESTION ) )
			RegenerateFileList( FL_Regeneration );
	}
	else
	{	// undo operation from FROM to TO stacks
		rToStack.push_front( rFromStack.front() );		// transfer FROM->TO by value
		rFromStack.pop_front();

		CAlbumImageView* pAlbumViewTarget = GetOwnActiveAlbumView();
		ASSERT_PTR( pAlbumViewTarget );
		// for the views != than the target view (if any), backup current/near selection before modifying the m_fileList
		UpdateAllViewsOfType< CAlbumImageView >( pAlbumViewTarget, Hint_SmartBackupSelection );

		custom_order::COpStep step = rToStack.front();		// copy by value since redo modifies it

		if ( isUndoOp )
			m_fileList.MoveBackCustomOrderIndexes( step.m_newDestIndex, step.m_toMoveIndexes );
		else
			m_fileList.MoveCustomOrderIndexes( step.m_toDestIndex, step.m_toMoveIndexes );

		if ( isUndoOp )
		{
			if ( rFromStack.empty() )
				SetModifiedFlag( Clean );
		}
		else
			SetModifiedFlag( Dirty );

		OnFileListChanged( FL_CustomOrderChanged );

		UpdateAllViewsOfType< CAlbumImageView >( pAlbumViewTarget, Hint_RestoreSelection );

		// select the un-dropped images
		CListViewState dropState( step.m_toMoveIndexes );

		pAlbumViewTarget->GetPeerThumbView()->SetListViewState( dropState, true );
		pAlbumViewTarget->OnUpdate( NULL, 0, NULL );
	}
	return true;
}

bool CAlbumDoc::InitAutoDropRecipient( void )
{
	CSearchSpec dropRecSearchSpec;
	bool success = m_fileList.IsAutoDropRecipient( false );

	if ( success )
		dropRecSearchSpec = m_fileList.GetAutoDropSearchSpec();
	else
		m_autoDropContext.Clear();

	return m_autoDropContext.InitAutoDropRecipient( dropRecSearchSpec );		// init auto-drop context's search attribute
}

// typically called when auto-drop turned off -> clear the undo buffer and all related data
void CAlbumDoc::ClearAutoDropContext( void )
{
	m_dropUndoStack.clear();
	m_dropRedoStack.clear();
	m_autoDropContext.Clear();
}

bool CAlbumDoc::HandleDropRecipientFiles( HDROP hDropInfo, CAlbumImageView* pTargetAlbumView )
{
	ASSERT_PTR( pTargetAlbumView );
	ASSERT( m_fileList.IsAutoDropRecipient( false ) && pTargetAlbumView->IsDropTargetEnabled() );
	if ( !m_fileList.IsAutoDropRecipient( true /*with validity check*/ ) )
	{
		AfxMessageBox( IDS_ERROR_INVALID_DEST_DIR );
		return false;
	}

	AfxGetMainWnd()->SetActiveWindow();							// activate the main frame first
	checked_static_cast< CMDIChildWnd* >( pTargetAlbumView->GetParentFrame() )->MDIActivate();

	int insertBeforeIndex = pTargetAlbumView->GetPeerThumbView()->GetPointedImageIndex();
	fs::CFlexPath insertBefore;

	if ( insertBeforeIndex != -1 )
		insertBefore = m_fileList.GetFileAttr( insertBeforeIndex ).GetPath();

	if ( m_autoDropContext.SetupDroppedFiles( hDropInfo, insertBefore ) > 0 )
		if ( m_autoDropContext.m_dropOperation == auto_drop::CContext::PromptUser )
			// do drop files on menu command
			m_autoDropContext.s_dropContextMenu.TrackPopupMenu( TPM_RIGHTBUTTON,
				m_autoDropContext.m_dropScreenPos.x,
				m_autoDropContext.m_dropScreenPos.y,
				AfxGetMainWnd() );
		else
			ExecuteAutoDrop();

	return true;
}

bool CAlbumDoc::ExecuteAutoDrop( void )
{
	if ( !m_autoDropContext.MakeAutoDrop( m_dropUndoStack ) )
		return false;

	RegenerateFileList( FL_AutoDropOp );
	return true;
}

void CAlbumDoc::OnFileListChanged( FileListChangeType reason /*= FL_Init*/ )
{
	InitAutoDropRecipient();

	if ( FL_AutoDropOp == reason )
	{	// also clear the thumb and image caches
		app::GetThumbnailer()->Clear();

		CWicImageCache::Instance().DiscardWithPrefix( m_autoDropContext.m_destSearchSpec.m_searchPath.GetPtr() );
	}

	UpdateAllViews( NULL, Hint_FileListChanged, app::ToHintPtr( reason ) );
}

void CAlbumDoc::OnAutoDropRecipientChanged( void )
{
	for ( POSITION pos = GetFirstViewPosition(); pos != NULL; )
		if ( CAlbumImageView* pAlbumView = dynamic_cast< CAlbumImageView* >( GetNextView( pos ) ) )
			pAlbumView->OnAutoDropRecipientChanged();
}

bool CAlbumDoc::AddExplicitFiles( const std::vector< std::tstring >& files, bool doUpdate /*= true*/ )
{
	if ( m_fileList.IsAutoDropRecipient( false ) )
		return false;

	// add search attributes for each explicit file or directory
	for ( std::vector< std::tstring >::const_iterator it = files.begin(); it != files.end(); ++it )
		m_fileList.m_searchSpecs.push_back( CSearchSpec( fs::CPath( it->c_str() ) ) );

	try
	{
		m_fileList.SearchForFiles();
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}

	m_slideData.SetCurrentIndex( 0 );
	SetModifiedFlag( Dirty );		// mark document as modified in order to prompt for saving.

	if ( doUpdate )
		OnFileListChanged();
	return true;
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumDoc, CDocumentBase )
	ON_COMMAND( CM_AUTO_DROP_DEFRAGMENT, CmAutoDropDefragment )
	ON_UPDATE_COMMAND_UI( CM_AUTO_DROP_DEFRAGMENT, OnUpdateAutoDropDefragment )
	ON_COMMAND( CM_AUTO_DROP_UNDO, CmAutoDropUndo )
	ON_UPDATE_COMMAND_UI( CM_AUTO_DROP_UNDO, OnUpdateAutoDropUndo )
	ON_COMMAND( CM_AUTO_DROP_REDO, CmAutoDropRedo )
	ON_UPDATE_COMMAND_UI( CM_AUTO_DROP_REDO, OnUpdateAutoDropRedo )
	ON_COMMAND( CM_AUTO_DROP_CLEAR_UNDO_REDO_STACKS, CmAutoDropClearUndoRedoStacks )
	ON_UPDATE_COMMAND_UI( CM_AUTO_DROP_CLEAR_UNDO_REDO_STACKS, OnUpdateAutoDropClearUndoRedoStacks )
	ON_COMMAND( CM_REGENERATE_ALBUM, CmRegenerateAlbum )
	ON_UPDATE_COMMAND_UI( CM_REGENERATE_ALBUM, OnUpdateRegenerateAlbum )
	ON_COMMAND( CM_IS_AUTO_DROP, OnToggleIsAutoDrop )
	ON_UPDATE_COMMAND_UI( CM_IS_AUTO_DROP, OnUpdateIsAutoDrop )
	ON_COMMAND( CM_CUSTOM_ORDER_UNDO, CmCustomOrderUndo )
	ON_UPDATE_COMMAND_UI( CM_CUSTOM_ORDER_UNDO, OnUpdateCustomOrderUndo )
	ON_COMMAND( CM_CUSTOM_ORDER_REDO, CmCustomOrderRedo )
	ON_UPDATE_COMMAND_UI( CM_CUSTOM_ORDER_REDO, OnUpdateCustomOrderRedo )
	ON_COMMAND( CM_CLEAR_CUSTOM_ORDER_UNDO_REDO_STACKS, CmClearCustomOrderUndoRedoStacks )
	ON_UPDATE_COMMAND_UI( CM_CLEAR_CUSTOM_ORDER_UNDO_REDO_STACKS, OnUpdateClearCustomOrderUndoRedoStacks )
	ON_COMMAND( CM_ARCHIVE_IMAGES, CmArchiveImages )
	ON_UPDATE_COMMAND_UI( CM_ARCHIVE_IMAGES, OnUpdateArchiveImages )
	ON_COMMAND( ID_EDIT_ARCHIVE_PASSWORD, OnEditArchivePassword )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ARCHIVE_PASSWORD, OnUpdateEditArchivePassword )
	ON_COMMAND( CK_SAVE_CU_UNDO_REDO_BUFFER, OnToggleSaveCOUndoRedoBuffer )
	ON_UPDATE_COMMAND_UI( CK_SAVE_CU_UNDO_REDO_BUFFER, OnUpdateSaveCOUndoRedoBuffer )
	ON_COMMAND( CM_SELECT_ALL_THUMBS, CmSelectAllThumbs )
	ON_UPDATE_COMMAND_UI( CM_SELECT_ALL_THUMBS, OnUpdateSelectAllThumbs )
END_MESSAGE_MAP()

BOOL CAlbumDoc::OnOpenDocument( LPCTSTR pPath )
{
	DeleteContents();
	SetModifiedFlag( DirtyOpening );			// dirty during loading, but prevent premature view updates

	const fs::CPath filePath( pPath );

	switch ( app::CAlbumDocTemplate::GetOpenPathType( filePath.GetPtr() ) )
	{
		case app::CAlbumDocTemplate::SlideAlbum:
			if ( !CDocumentBase::OnOpenDocument( pPath ) )
				return FALSE;
			SetModifiedFlag( Clean );					// start off clean
			return TRUE;
		case app::CAlbumDocTemplate::DirPath:
		case app::CAlbumDocTemplate::ImageArchiveDoc:
			return BuildAlbum( filePath );
		default:
			app::GetUserReport().MessageBox( str::Format( _T("Cannot open unrecognized album file:\n\n%s"), filePath.GetPtr() ) );
			return FALSE;
	}
}

BOOL CAlbumDoc::OnSaveDocument( LPCTSTR pPathName )
{
	if ( app::IsImageArchiveDoc( pPathName ) )
		return SaveAsArchiveStg( fs::CPath( pPathName ) );
	else if ( fs::IsValidDirectory( pPathName ) )
		return DoSave( NULL );					// in effect Save As

	return CDocumentBase::OnSaveDocument( pPathName );
}

void CAlbumDoc::OnCloseDocument( void )
{
	CDocumentBase::OnCloseDocument();
}

void CAlbumDoc::CmAutoDropDefragment( void )
{
	if ( m_autoDropContext.DefragmentFiles( m_dropUndoStack ) )
		RegenerateFileList( FL_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropDefragment( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_fileList.IsAutoDropRecipient( true ) );
}

void CAlbumDoc::CmAutoDropUndo( void )
{
	// undo operation from UNDO to REDO staks
	if ( m_autoDropContext.UndoRedoOperation( m_dropUndoStack, m_dropRedoStack, true ) )
		RegenerateFileList( FL_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropUndo( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_fileList.IsAutoDropRecipient( true ) && m_dropUndoStack.size() > 0 );
}

void CAlbumDoc::CmAutoDropRedo( void )
{
	// redo operation from REDO to UNDO stacks
	if ( m_autoDropContext.UndoRedoOperation( m_dropRedoStack, m_dropUndoStack, false ) )
		RegenerateFileList( FL_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropRedo( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_fileList.IsAutoDropRecipient( true ) && m_dropRedoStack.size() > 0 );
}

void CAlbumDoc::CmAutoDropClearUndoRedoStacks( void )
{
	ClearAutoDropContext();
}

void CAlbumDoc::OnUpdateAutoDropClearUndoRedoStacks( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_dropUndoStack.size() > 0 || m_dropRedoStack.size() > 0 );
}

void CAlbumDoc::CmRegenerateAlbum( void )
{
	RegenerateFileList( FL_Regeneration );
}

void CAlbumDoc::OnUpdateRegenerateAlbum( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( TRUE );
}

void CAlbumDoc::OnToggleIsAutoDrop( void )
{
	ASSERT( m_fileList.GetSearchSpecCount() == 1 && m_fileList.m_searchSpecs.front().IsDirPath() );

	CWaitCursor wait;
	CSearchSpec& searchSpec = m_fileList.m_searchSpecs.front();

	if ( !m_fileList.IsAutoDropRecipient( false ) )
		searchSpec.m_options = CSearchSpec::AutoDropNumFormat;		// turn auto-drop ON
	else
		searchSpec.m_options = CSearchSpec::RecurseSubDirs;		// turn auto-drop OFF

	// regenerate the file list
	try
	{
		m_fileList.SearchForFiles();
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
	}
	m_slideData.SetCurrentIndex( 0 );						// reset the selection

	// clear auto-drop context since the drop directory may have changed
	ClearAutoDropContext();

	SetModifiedFlag();		// mark document as modified in order to prompt for saving

	OnFileListChanged();
	OnAutoDropRecipientChanged();
}

void CAlbumDoc::OnUpdateIsAutoDrop( CCmdUI* pCmdUI )
{
	bool enable = 1 == m_fileList.GetSearchSpecCount();

	if ( enable )
	{
		CSearchSpec& searchSpec = m_fileList.m_searchSpecs.front();

		enable = searchSpec.IsDirPath() && searchSpec.IsValidPath();
	}
	pCmdUI->Enable( enable );
	pCmdUI->SetCheck( m_fileList.IsAutoDropRecipient( false ) );
}

void CAlbumDoc::CmCustomOrderUndo( void )
{
	UndoRedoCustomOrder( m_customOrderUndoStack, m_customOrderRedoStack, true );
}

void CAlbumDoc::OnUpdateCustomOrderUndo( CCmdUI* pCmdUI )
{
	// enable whether or not in custom order mode since we might have file operations to undo
	pCmdUI->Enable( !m_customOrderUndoStack.empty() );
	pCmdUI->SetText( str::Format( _T("&Undo %s\tAlt+Bksp"),
		m_customOrderUndoStack.empty() ? _T("") : m_customOrderUndoStack.front().GetOperationTag().c_str() ).c_str() );
}

void CAlbumDoc::CmCustomOrderRedo( void )
{
	UndoRedoCustomOrder( m_customOrderRedoStack, m_customOrderUndoStack, false );
}

void CAlbumDoc::OnUpdateCustomOrderRedo( CCmdUI* pCmdUI )
{
	// enable whether or not in custom order mode since we might have file operations to redo
	pCmdUI->Enable( !m_customOrderRedoStack.empty() );
	pCmdUI->SetText( str::Format( _T("&Redo %s\tAlt+Ins"),
		m_customOrderRedoStack.empty() ? _T("") : m_customOrderRedoStack.front().GetOperationTag().c_str() ).c_str() );
}

void CAlbumDoc::CmClearCustomOrderUndoRedoStacks( void )
{
	ClearCustomOrder();
}

void CAlbumDoc::OnUpdateClearCustomOrderUndoRedoStacks( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_customOrderUndoStack.empty() || !m_customOrderRedoStack.empty() );
}

void CAlbumDoc::CmArchiveImages( void )
{
	CArchiveImagesDialog dialog( &m_fileList, GetPathName().GetString() );
	if ( CAlbumImageView* pActiveView = GetOwnActiveAlbumView() )
	{
		CListViewState lvState( StoreByIndex );

		pActiveView->GetPeerThumbView()->GetListViewState( lvState );
		dialog.StoreSelection( lvState );
	}
	if ( dialog.DoModal() != IDCANCEL )
	{
		if ( ToDirectory == dialog.m_destType )
		{
			custom_order::COpStep step;

			step.m_fileOp = dialog.m_fileOp;
			step.m_archivedImages = dialog.m_filesContext;

			// push to the undo stack the current custom order step
			m_customOrderUndoStack.push_front( step );

			// after custom order must clear the redo stack
			m_customOrderRedoStack.ClearStack();
		}

		if ( FOP_FileMove == dialog.m_fileOp )			// current file set may have changed?
			if ( IDOK == AfxMessageBox( IDS_PROMPT_REGENERATE_ALBUM, MB_OKCANCEL | MB_ICONQUESTION ) )
				RegenerateFileList( FL_Regeneration );
	}
}

void CAlbumDoc::OnUpdateArchiveImages( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( HasImages() );
}

void CAlbumDoc::OnEditArchivePassword( void )
{
	fs::CPath stgFilePath( GetPathName().GetString() );
	ASSERT( app::IsImageArchiveDoc( stgFilePath.GetPtr() ) );

	if ( stgFilePath.FileExist() )
	{
		std::tstring password = CImageArchiveStg::Factory().LoadPassword( stgFilePath );
		CImageArchiveStg::DecryptPassword( password );

		CDefinePasswordDialog dlg( stgFilePath.GetNameExt() );
		dlg.m_password = dlg.m_confirmPassword = password;

		if ( dlg.Run() )
		{
			CImageArchiveStg::EncryptPassword( dlg.m_password );
			CImageArchiveStg::Factory().SavePassword( dlg.m_password, stgFilePath );
		}
	}
	else
		ui::BeepSignal( MB_ICONEXCLAMATION );
}

void CAlbumDoc::OnUpdateEditArchivePassword( CCmdUI* pCmdUI )
{
	fs::CPath docPath( GetPathName().GetString() );
	pCmdUI->Enable( docPath.FileExist() && app::IsImageArchiveDoc( docPath.GetPtr() ) );
}

void CAlbumDoc::OnToggleSaveCOUndoRedoBuffer( void )
{
	CSlideData* pSlideData = GetActiveSlideData();
	ToggleFlag( pSlideData->m_viewFlags, af::SaveCustomOrderUndoRedo );
}

void CAlbumDoc::OnUpdateSaveCOUndoRedoBuffer( CCmdUI* pCmdUI )
{
	const CSlideData* pSlideData = GetActiveSlideData();

	pCmdUI->Enable( TRUE );
	pCmdUI->SetCheck( HasFlag( pSlideData->m_viewFlags, af::SaveCustomOrderUndoRedo ) );
}

void CAlbumDoc::CmSelectAllThumbs( void )
{
	if ( CAlbumImageView* pActiveView = GetOwnActiveAlbumView() )
		pActiveView->GetPeerThumbView()->SelectAll();
}

void CAlbumDoc::OnUpdateSelectAllThumbs( CCmdUI* pCmdUI )
{
	CAlbumImageView* pActiveView = GetOwnActiveAlbumView();
	pCmdUI->Enable( pActiveView != NULL && HasFlag( pActiveView->GetSlideData().m_viewFlags, af::ShowThumbView ) );
}
