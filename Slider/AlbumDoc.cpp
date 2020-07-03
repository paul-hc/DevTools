
#include "stdafx.h"
#include "AlbumDoc.h"
#include "Workspace.h"
#include "DocTemplates.h"
#include "SearchPattern.h"
#include "FileAttr.h"
#include "AlbumSettingsDialog.h"
#include "CatalogStorageService.h"
#include "ProgressService.h"
  #include "ArchiveImagesDialog.h"
#include "AlbumImageView.h"
#include "AlbumThumbListView.h"
#include "ICatalogStorage.h"
#include "FileOperation.h"
#include "Application.h"
#include "resource.h"
#include "utl/MemLeakCheck.h"
#include "utl/Serialization.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/IconButton.h"
#include "utl/UI/PasswordDialog.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumDoc, CDocumentBase )

CAlbumDoc::CAlbumDoc( void )
	: CDocumentBase()
	, m_bkColor( CLR_DEFAULT )
	, m_docFlags( 0 )
{
}

CAlbumDoc::~CAlbumDoc()
{
	m_model.CloseAllStorages();		// redundant, it happens anyway since the model manages open storages
}

void CAlbumDoc::DeleteContents( void )
{
	__super::DeleteContents();		// does nothing

	m_model.Clear();
}

void CAlbumDoc::CopyAlbumState( const CAlbumDoc* pSrcDoc )
{
	REQUIRE( this != pSrcDoc );
	ASSERT_PTR( pSrcDoc );

	const_cast< CAlbumDoc* >( pSrcDoc )->FetchViewState();		// input image state from its current view

	m_slideData = pSrcDoc->m_slideData;
	m_bkColor = pSrcDoc->m_bkColor;
	m_pImageState.reset( pSrcDoc->m_pImageState.get() != NULL ? new CImageState( *pSrcDoc->m_pImageState ) : NULL );
	m_password = pSrcDoc->m_password;
}

void CAlbumDoc::FetchViewState( void )
{
	if ( CAlbumImageView* pImageView = GetAlbumImageView() )
	{
		// explicitly copy the persistent attributes from active view
		m_slideData = pImageView->GetSlideData();
		m_bkColor = pImageView->GetRawBkColor();

		m_pImageState.reset( new CImageState );
		pImageView->MakeImageState( m_pImageState.get() );
	}
}

void CAlbumDoc::Serialize( CArchive& archive )
{
	REQUIRE( serial::IsFileBasedArchive( archive ) );			// mem-based document serialization not supported/necessary (for now)

	fs::CPath docPath = serial::GetDocumentPath( archive );

	if ( archive.IsStoring() )
		PrepareToSave( docPath );

	serial::CStreamingTimeGuard timeGuard( archive );
	CWaitCursor wait;

	// version backwards compatibility hack: check if a valid version is saved as first UINT
	CSlideData::TFirstDataMember firstValue = UINT_MAX;
	std::auto_ptr< serial::CScopedLoadingArchive > pLoadingArchive;

	if ( archive.IsLoading() )
	{
		archive >> firstValue;

		app::ModelSchema docModelSchema = app::Slider_v3_1;										// assume an old backwards-compatible model schema (not saved back in the day)

		if ( firstValue >= app::Slider_v3_2 && firstValue <= app::Slider_LatestModelSchema )	// valid version saved?
		{	// let model details know the loading archive version
			docModelSchema = static_cast< app::ModelSchema >( firstValue );						// store original document model schema (required for further storage access/metadata lookups)
			firstValue = UINT_MAX;			// mark as extracted (as file ModelSchema)
		}

		pLoadingArchive.reset( new serial::CScopedLoadingArchive( archive, docModelSchema ) );

		m_model.StoreModelSchema( docModelSchema );									// data-member in model but persisted by this document

		if ( ICatalogStorage* pCatalogStorage = CCatalogStorageFactory::Instance()->FindStorage( docPath ) )
			pCatalogStorage->StoreDocModelSchema( docModelSchema );
	}
	else
	{
		ASSERT( app::Slider_LatestModelSchema == GetModelSchema() );				// always save the latest model schema version as first UINT in the archive
		archive << GetModelSchema();
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

	m_model.Stream( archive );

//	if ( archive.IsLoading() )
//		m_model._CheckReparentFileAttrs( docPath.GetPtr(), CAlbumModel::Loading );		// re-parent embedded image paths with current doc stg path

	serial::StreamItems( archive, m_dropUndoStack );
	serial::StreamItems( archive, m_dropRedoStack );
	InitAutoDropRecipient();

	if ( HasFlag( m_slideData.m_viewFlags, af::SaveCustomOrderUndoRedo ) )
	{
		serial::StreamItems( archive, m_customOrderUndoStack );
		serial::StreamItems( archive, m_customOrderRedoStack );
	}

	if ( HasFlag( m_docFlags, PresistImageState ) )
		serial::StreamPtr( archive, m_pImageState );

	if ( archive.IsStoring() )
		SetModifiedFlag( Clean );

	app::LogLine( _T("%s album \"%s\" with model schema version %s - elapsed %s"),
		archive.IsLoading() ? _T("Loaded") : _T("Saved"),
		docPath.GetPtr(),
		app::FormatSliderVersion( GetModelSchema() ).c_str(),
		timeGuard.GetTimer().FormatElapsedDuration( 2 ).c_str() );
}

void CAlbumDoc::PrepareToSave( const fs::CPath& docPath )
{
	SetFlag( m_docFlags, PresistImageState, HasFlag( CWorkspace::GetFlags(), wf::PersistAlbumImageState ) );
	FetchViewState();

	if ( GetModelSchema() != app::Slider_LatestModelSchema )
	{
		if ( path::EquivalentPtr( m_strPathName, docPath.GetPtr() ) )		// Save? (not Save As)
			if ( !PromptSaveConvertModelSchema() )
				throw new mfc::CUserAbortedException;

		m_model.StoreModelSchema( app::Slider_LatestModelSchema );			// save with latest model schema format
	}
}

bool CAlbumDoc::PromptSaveConvertModelSchema( void ) const
{
	const bool is_Album_sld = app::IsCatalogFile( m_strPathName );
	std::tstring message = str::Format( _T("Save older %s album document to the latest version %s%c\n\n%s"),
		app::FormatModelVersion( GetModelSchema() ).c_str(),
		app::FormatModelVersion( app::Slider_LatestModelSchema ).c_str(),
		is_Album_sld ? _T('!') : _T('?'),
		m_strPathName );

	bool proceed = true;
#ifdef _DEBUG
	// skip this annoying question in release build
	if ( is_Album_sld )
		ui::MessageBox( message, MB_OK | MB_ICONINFORMATION );		// saving "_Album.sld" in structured storage: user can't cancel saving (it would corrupt the embedded album file)
	else
		proceed = IDOK == ui::MessageBox( message, MB_OKCANCEL | MB_ICONQUESTION );
#endif
	if ( !proceed )
		message += _T("\n CANCELLED by user!");

	str::Replace( message, _T("\n\n"), _T("\n  "), 1 );				// compact the lines for logging
	app::LogLine( message.c_str() );
	return proceed;
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
		return m_model.GetFileAttr( index )->GetPathKey();

	return CWicImage::s_nullKey;
}

CAlbumImageView* CAlbumDoc::GetAlbumImageView( void ) const
{
	return ui::FindDocumentView< CAlbumImageView >( this );
}

CSlideData* CAlbumDoc::GetActiveSlideData( void )
{
	CAlbumImageView* pImageView = GetAlbumImageView();
	return pImageView != NULL ? &pImageView->RefSlideData() : &m_slideData;
}


bool CAlbumDoc::IsStorageAlbum( void ) const
{
	return CCatalogStorageFactory::HasCatalogExt( GetDocFilePath().GetPtr() );
}

ICatalogStorage* CAlbumDoc::GetCatalogStorage( void )
{
	if ( IsStorageAlbum() )
		return m_model.GetCatalogStorage();

	return NULL;
}

std::auto_ptr< CAlbumDoc > CAlbumDoc::LoadCatalogStorageAlbum( const fs::CPath& docStgPath )
{
	std::auto_ptr< CAlbumDoc > pNewAlbumDoc( new CAlbumDoc() );
	CComPtr< ICatalogStorage > pCatalogStorage = CCatalogStorageFactory::Instance()->AcquireStorage( docStgPath, STGM_READ );

	if ( !pNewAlbumDoc->LoadCatalogStorage( docStgPath ) )
		pNewAlbumDoc.reset();

	return pNewAlbumDoc;
}

bool CAlbumDoc::LoadCatalogStorage( const fs::CPath& docStgPath )
{
	ASSERT( app::IsCatalogFile( docStgPath.GetPtr() ) );

	CComPtr< ICatalogStorage > pCatalogStorage = CCatalogStorageFactory::Instance()->AcquireStorage( docStgPath, STGM_READ );

	if ( NULL == pCatalogStorage )
		return false;

	if ( !CCatalogPasswordStore::Instance()->LoadPasswordVerify( &m_password, pCatalogStorage ) )
		return false;

	m_model.StoreCatalogDocPath( docStgPath );

	// note: album stream is optional for older archives: not an error if missing

	if ( !pCatalogStorage->LoadAlbumStream( this ) )
		return false;

	m_model.OpenAllStorages();
	return true;				// succeeded
}

bool CAlbumDoc::SaveAsCatalogStorage( const fs::CPath& newDocStgPath )
{
	REQUIRE( app::IsCatalogFile( newDocStgPath.GetPtr() ) );

	ui::IProgressService* pProgressSvc = ui::CNoProgressService::Instance();		// default for unit testing
	std::auto_ptr< CProgressService > pProgress;

	if ( GetAlbumImageView() != NULL )				// normal interactive mode?
	{
		pProgress.reset( new CProgressService( NULL, _T("Creating image archive storage file") ) );
		pProgressSvc = pProgress->GetService();
	}

	try
	{
		fs::CPath oldDocStgPath = GetDocFilePath();

		if ( oldDocStgPath.IsEmpty() ||										// never saved
			 !fs::IsValidStructuredStorage( oldDocStgPath.GetPtr() ) ||		// was a .sld album
			 newDocStgPath != oldDocStgPath ||								// SaveAs
			 GetModelSchema() < app::Slider_LatestModelSchema )				// loaded catalog with older model schema -> must be converted to LATEST
		{
			{
				CCatalogStorageService storageSvc( pProgressSvc, &app::GetUserReport() );		// catalog storage metadata

				storageSvc.BuildFromAlbumSaveAs( this );

				CComPtr< ICatalogStorage > pCatalogStorage = CCatalogStorageFactory::CreateStorageObject();

				pCatalogStorage->CreateImageArchiveFile( newDocStgPath, &storageSvc );			// SaveAs: create the entire image catalog - works internally in utl::ThrowMode
			}

			// reload the newly saved document - takes care of saving .sld to .ias
			DeleteContents();
			BuildAlbum( CSearchPattern( newDocStgPath ) );
		}
		else
		{
			ICatalogStorage* pCatalogStorage = GetCatalogStorage();
			fs::stg::CScopedWriteDocMode scopedDocWrite( pCatalogStorage->GetDocStorage(), NULL );		// switch storage to write/throw mode

			pCatalogStorage->SavePasswordStream();
			pCatalogStorage->SaveAlbumStream( this );
		}
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}

	return true;
}

bool CAlbumDoc::_InternalSaveAsArchiveStg( const fs::CPath& newDocStgPath )
{
	REQUIRE( app::IsCatalogFile( newDocStgPath.GetPtr() ) );

	try
	{
		fs::CPath oldDocPath( GetPathName().GetString() );
		bool saveAs = newDocStgPath != oldDocPath;

		if ( saveAs )
		{
			bool straightStgFileCopy = false;

			if ( !IsModified() )												// in synch with the file?
				if ( app::Slider_LatestModelSchema == GetModelSchema() )		// latest model schema? (will always save with Slider_LatestModelSchema)
					if ( app::IsCatalogFile( oldDocPath.GetPtr() ) )
						if ( m_model.HasConsistentDeepStreams() )
							straightStgFileCopy = true;

			if ( straightStgFileCopy )
			{
				CFileOperation fileOp( utl::ThrowMode );

				fileOp.Copy( fs::ToFlexPath( oldDocPath ), fs::ToFlexPath( newDocStgPath ) );		// optimization: straight stg file copy
				fs::MakeFileWritable( newDocStgPath.GetPtr() );									// just in case source was read-only
			}
			else
			{
//				if ( !oldDocPath.IsEmpty() )
//					CImageCatalogStg::DiscardCachedImages( oldDocPath );

				CArchivingModel archivingModel;
				archivingModel.StorePassword( m_password );

				CAlbumModel tempModel = m_model;				// temp copy so that it can display original thumbnails while creating, avoiding sharing errors

				// don't pass the document, it's too early to save the album stream, since we're working on a tempModel copy
				if ( archivingModel.CreateArchiveStgFile( &tempModel, newDocStgPath ) )
					m_model = tempModel;						// assign the results
				else
					return false;
			}
		}

		_SaveAlbumToArchiveStg( newDocStgPath );
		if ( !saveAs )
			return true;

		return BuildAlbum( newDocStgPath );						// reload from the new archive document file so that we reinitialize m_model
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}
}

void CAlbumDoc::_SaveAlbumToArchiveStg( const fs::CPath& docStgPath ) throws_( CException* )
{
	// save existing album to image archive as "_Album.sld" stream
//	CScopedErrorHandling scopedThrow( CImageCatalogStg::Factory(), utl::ThrowMode );

	m_model._CheckReparentFileAttrs( docStgPath.GetPtr(), CAlbumModel::Saving );		// reparent with docStgPath before saving the album info
/*
	if ( CImageCatalogStg::Factory()->SaveAlbumStream( this, docStgPath ) )
		if ( ICatalogStorage* pSavedImageStg = CImageCatalogStg::Factory()->FindStorage( docStgPath ) )
			pSavedImageStg->StoreDocModelSchema( GetModelSchema() );
*/
}

bool CAlbumDoc::BuildAlbum( const CSearchPattern& searchPattern )
{
	const fs::CPath& searchPath = searchPattern.GetFilePath();
	bool opening = DirtyOpening == IsModified();
	bool loadedStgAlbumStream = false;

	m_slideData.SetCurrentIndex( 0 );				// may get overridden by subsequent load of album doc
	try
	{
		if ( app::IsCatalogFile( searchPath.GetPtr() ) )
		{
			if ( LoadCatalogStorage( searchPath ) )
				loadedStgAlbumStream = true;
			else
				return false;
		}
		else
		{
			if ( m_model.SetupSingleSearchPattern( searchPattern ) )
			{
				m_model.SearchForFiles( NULL );
				m_model.OpenAllStorages();		// to enable image caching
			}
			else
				return false;
		}
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}

	SetModifiedFlag( Clean );

	if ( !opening )					// not too early for view updates?
	{
		OnAlbumModelChanged();		// update the UI

		if ( loadedStgAlbumStream )
			UpdateAllViews( NULL, Hint_DocSlideDataChanged );		// refresh view navigation from document (selected pos, etc)
	}

	return true;		// keep it open regardless /*m_model.AnyFoundFiles();*/
}

void CAlbumDoc::RegenerateModel( AlbumModelChange reason /*= FM_Init*/ )
{
	if ( FM_Regeneration == reason )
		UpdateAllViewsOfType< CAlbumImageView >( NULL, Hint_BackupCurrSelection );		// backup current selection for all the owned views before re-generating the m_model member

	// We can't rely on reordering information since there might be new or removed files.
	// However, we can keep file copy/move operations in undo/redo buffers.
	ClearCustomOrder( custom_order::CM_ClearReorder );
	try
	{
		CWaitCursor	wait;
		m_model.SearchForFiles( NULL, false );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
	}
	SetModifiedFlag( Dirty );

	// update UI
	OnAlbumModelChanged( reason );

	if ( FM_Regeneration == reason )
		UpdateAllViewsOfType< CAlbumImageView >( NULL, Hint_RestoreSelection );		// restore backed-up selection for all the views

	if ( FM_AutoDropOp == reason )
		// since auto-drop operation is usually done from opened albums (as well as Windows Explorer),
		// also redraw any of the opened views that may be affected by this, except for those views
		// belonging to this document, which is just updated.
		app::GetApp()->UpdateAllViews( Hint_FileChanged, this );
}

bool CAlbumDoc::EditAlbum( CAlbumImageView* pActiveView )
{
	CAlbumSettingsDialog dlg( m_model, pActiveView->GetSlideData().GetCurrentIndex(), pActiveView );
	if ( dlg.DoModal() != IDOK )
		return false;

	m_model = dlg.GetModel();
	m_slideData.SetCurrentIndex( dlg.GetCurrentIndex() );
	// clear auto-drop context since the drop directory may have changed
	ClearAutoDropContext();

	SetModifiedFlag( Dirty );		// mark document as modified in order to prompt for saving

	OnAlbumModelChanged();
	OnAutoDropRecipientChanged();
	return true;
}

void CAlbumDoc::ClearCustomOrder( custom_order::ClearMode clearMode /*= custom_order::CM_ClearAll*/ )
{
	m_customOrderUndoStack.ClearStack( clearMode );
	m_customOrderRedoStack.ClearStack( clearMode );
}

bool CAlbumDoc::DropCustomOrder( int& rDropIndex, std::vector< int >& rSelIndexes )
{
	custom_order::COpStep step;

	step.m_dragSelIndexes = rSelIndexes;
	step.m_dropIndex = rDropIndex;

	if ( !m_model.DropCustomOrderIndexes( rDropIndex, rSelIndexes ) )
		return false;

	step.m_newDroppedIndex = rDropIndex;
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
			FOP_FileCopy == topStep.m_fileOp ? _T("copy") : _T("move") );

		if ( AfxMessageBox( message.c_str(), MB_OKCANCEL | MB_ICONQUESTION ) != IDOK )
			return false;

		CArchivingModel& rArchivingModel = topStep.RefArchivingModel();
		std::vector< TTransferPathPair > errorPairs;

		if ( !rArchivingModel.CanCommitOperations( errorPairs, topStep.m_fileOp, isUndoOp ) )
		{
			message = str::Format( IDS_UNDOREDO_INVALID_FILE_PAIRS,
				isUndoOp ? _T("undo") : _T("redo"),
				errorPairs.size(),
				FOP_FileCopy == topStep.m_fileOp ? _T("copy") : _T("move"),
				rArchivingModel.GetPathPairs().size() );
			switch ( AfxMessageBox( message.c_str(), MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
			{
				case IDABORT:	rFromStack.pop_front(); return false;		// remove the faulty entry from the stack
				case IDRETRY:	return false;
				case IDIGNORE:	break;
			}
		}

		if ( !rArchivingModel.CommitOperations( topStep.m_fileOp, isUndoOp ) )
			return false;

		// undo operation from FROM to TO stacks
		rToStack.push_front( rFromStack.front() );		// transfer FROM->TO by value
		rFromStack.pop_front();

		if ( IDOK == AfxMessageBox( IDS_PROMPT_REGENERATE_ALBUM, MB_OKCANCEL | MB_ICONQUESTION ) )
			RegenerateModel( FM_Regeneration );
	}
	else
	{	// undo operation from FROM to TO stacks
		rToStack.push_front( rFromStack.front() );		// transfer FROM->TO by value
		rFromStack.pop_front();

		CAlbumImageView* pAlbumViewTarget = GetAlbumImageView();
		ASSERT_PTR( pAlbumViewTarget );
		// for the views != than the target view (if any), backup current/near selection before modifying the m_model
		UpdateAllViewsOfType< CAlbumImageView >( pAlbumViewTarget, Hint_SmartBackupSelection );

		custom_order::COpStep step = rToStack.front();		// copy by value since redo modifies it

		if ( isUndoOp )
			m_model.UndropCustomOrderIndexes( step.m_newDroppedIndex, step.m_dragSelIndexes );
		else
			m_model.DropCustomOrderIndexes( step.m_dropIndex, step.m_dragSelIndexes );

		if ( isUndoOp )
		{
			if ( rFromStack.empty() )
				SetModifiedFlag( Clean );
		}
		else
			SetModifiedFlag( Dirty );

		OnAlbumModelChanged( FM_CustomOrderChanged );

		UpdateAllViewsOfType< CAlbumImageView >( pAlbumViewTarget, Hint_RestoreSelection );

		// select the un-dropped images
		CListViewState dropState( step.m_dragSelIndexes );
		dropState.SetCaretOnSel();			// ensure that caret is visible: put caret on first selected item

		pAlbumViewTarget->GetPeerThumbView()->SetListViewState( dropState, true );
		pAlbumViewTarget->OnUpdate( NULL, 0, NULL );
	}
	return true;
}

bool CAlbumDoc::InitAutoDropRecipient( void )
{
	CSearchPattern dropRecSearchPattern;

	if ( m_model.IsAutoDropRecipient( false ) )
		dropRecSearchPattern = *m_model.RefSearchModel()->RefSinglePattern();
	else
		m_autoDropContext.Clear();

	return m_autoDropContext.InitAutoDropRecipient( dropRecSearchPattern );		// init auto-drop context's search attribute
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
	ASSERT( m_model.IsAutoDropRecipient( false ) && pTargetAlbumView->IsDropTargetEnabled() );
	if ( !m_model.IsAutoDropRecipient( true /*with validity check*/ ) )
	{
		AfxMessageBox( IDS_ERROR_INVALID_DEST_DIR );
		return false;
	}

	AfxGetMainWnd()->SetActiveWindow();							// activate the main frame first
	checked_static_cast< CMDIChildWnd* >( pTargetAlbumView->GetParentFrame() )->MDIActivate();

	int insertBeforeIndex = pTargetAlbumView->GetPeerThumbView()->GetPointedImageIndex();
	fs::CFlexPath insertBefore;

	if ( insertBeforeIndex != -1 )
		insertBefore = m_model.GetFileAttr( insertBeforeIndex )->GetPath();

	if ( m_autoDropContext.SetupDroppedFiles( hDropInfo, insertBefore ) > 0 )
		if ( auto_drop::CContext::PromptUser == m_autoDropContext.GetDropOperation() )
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

	RegenerateModel( FM_AutoDropOp );
	return true;
}

void CAlbumDoc::OnAlbumModelChanged( AlbumModelChange reason /*= FM_Init*/ )
{
	InitAutoDropRecipient();

	if ( FM_AutoDropOp == reason )
	{	// also clear the thumb and image caches
		app::GetThumbnailer()->Clear();

		CWicImageCache::Instance().DiscardWithPrefix( m_autoDropContext.GetDestSearchPath().GetPtr() );
	}

	UpdateAllViews( NULL, Hint_AlbumModelChanged, app::ToHintPtr( reason ) );
}

void CAlbumDoc::OnAutoDropRecipientChanged( void )
{
	GetAlbumImageView()->OnAutoDropRecipientChanged();
}

bool CAlbumDoc::AddExplicitFiles( const std::vector< std::tstring >& files, bool doUpdate /*= true*/ )
{
	if ( m_model.IsAutoDropRecipient( false ) )
		return false;

	// add search attributes for each explicit file or directory
	for ( std::vector< std::tstring >::const_iterator it = files.begin(); it != files.end(); ++it )
		m_model.RefSearchModel()->AddSearchPath( *it );

	try
	{
		m_model.SearchForFiles( NULL );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}

	m_slideData.SetCurrentIndex( 0 );
	SetModifiedFlag( Dirty );			// mark document as modified in order to prompt for saving.

	if ( doUpdate )
		OnAlbumModelChanged();
	return true;
}


// MFC base overrides

BOOL CAlbumDoc::OnOpenDocument( LPCTSTR pPath )
{
	DeleteContents();
	SetModifiedFlag( DirtyOpening );			// dirty during loading, to prevent premature view updates

	const fs::CPath filePath( pPath );

	switch ( app::CAlbumDocTemplate::GetOpenPathType( filePath.GetPtr() ) )
	{
		case app::CAlbumDocTemplate::SlideAlbum:
			if ( !CDocumentBase::OnOpenDocument( pPath ) )
				return FALSE;

			m_model.OpenAllStorages();			// to enable image caching
			SetModifiedFlag( Clean );			// start off clean
			return TRUE;
		case app::CAlbumDocTemplate::DirPath:
		case app::CAlbumDocTemplate::CatalogStorageDoc:
			return BuildAlbum( filePath );
		default:
			app::GetUserReport().MessageBox( str::Format( _T("Cannot open unrecognized album file:\n\n%s"), filePath.GetPtr() ) );
			return FALSE;
	}
}

BOOL CAlbumDoc::OnSaveDocument( LPCTSTR pPathName )
{
	if ( app::IsCatalogFile( pPathName ) )
		return SaveAsCatalogStorage( fs::CPath( pPathName ) );
	else if ( fs::IsValidDirectory( pPathName ) )
		return DoSave( NULL );					// in effect SaveAs

	return CDocumentBase::OnSaveDocument( pPathName );
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

void CAlbumDoc::CmAutoDropDefragment( void )
{
	if ( m_autoDropContext.DefragmentFiles( m_dropUndoStack ) )
		RegenerateModel( FM_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropDefragment( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_model.IsAutoDropRecipient( true ) );
}

void CAlbumDoc::CmAutoDropUndo( void )
{
	// undo operation from UNDO to REDO staks
	if ( m_autoDropContext.UndoRedoOperation( m_dropUndoStack, m_dropRedoStack, true ) )
		RegenerateModel( FM_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropUndo( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_model.IsAutoDropRecipient( true ) && m_dropUndoStack.size() > 0 );
}

void CAlbumDoc::CmAutoDropRedo( void )
{
	// redo operation from REDO to UNDO stacks
	if ( m_autoDropContext.UndoRedoOperation( m_dropRedoStack, m_dropUndoStack, false ) )
		RegenerateModel( FM_AutoDropOp );
}

void CAlbumDoc::OnUpdateAutoDropRedo( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_model.IsAutoDropRecipient( true ) && m_dropRedoStack.size() > 0 );
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
	RegenerateModel( FM_Regeneration );
}

void CAlbumDoc::OnUpdateRegenerateAlbum( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( TRUE );
}

void CAlbumDoc::OnToggleIsAutoDrop( void )
{
	CSearchPattern* pSearchPattern = m_model.RefSearchModel()->RefSinglePattern();
	ASSERT_PTR( pSearchPattern );
	ASSERT( pSearchPattern->IsDirPath() );

	CWaitCursor wait;

	if ( !m_model.IsAutoDropRecipient( false ) )
		pSearchPattern->SetSearchMode( CSearchPattern::AutoDropNumFormat );		// turn auto-drop ON
	else
		pSearchPattern->SetSearchMode( CSearchPattern::RecurseSubDirs );			// turn auto-drop OFF

	// regenerate the file list
	try
	{
		m_model.SearchForFiles( NULL );
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
	}
	m_slideData.SetCurrentIndex( 0 );						// reset the selection

	// clear auto-drop context since the drop directory may have changed
	ClearAutoDropContext();

	SetModifiedFlag();		// mark document as modified in order to prompt for saving

	OnAlbumModelChanged();
	OnAutoDropRecipientChanged();
}

void CAlbumDoc::OnUpdateIsAutoDrop( CCmdUI* pCmdUI )
{
	bool enable = false;

	if ( const CSearchPattern* pSearchPattern = m_model.GetSearchModel()->GetSinglePattern() )
		enable = pSearchPattern->IsDirPath() && pSearchPattern->IsValidPath();

	pCmdUI->Enable( enable );
	pCmdUI->SetCheck( m_model.IsAutoDropRecipient( false ) );
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
	CArchiveImagesDialog dialog( &m_model, GetPathName().GetString() );

	if ( CAlbumImageView* pImageView = GetAlbumImageView() )
	{
		CListViewState lvState( StoreByIndex );

		pImageView->GetPeerThumbView()->GetListViewState( lvState );
		dialog.StoreSelection( lvState );
	}

	if ( dialog.DoModal() != IDCANCEL )
	{
		if ( ToDirectory == dialog.m_destType )
		{
			custom_order::COpStep step;

			step.m_fileOp = dialog.m_fileOp;
			step.StoreArchivingModel( dialog.m_archivingModel );

			// push to the undo stack the current custom order step
			m_customOrderUndoStack.push_front( step );

			// after custom order must clear the redo stack
			m_customOrderRedoStack.ClearStack();
		}

		if ( FOP_FileMove == dialog.m_fileOp )			// current file set may have changed?
			if ( IDOK == AfxMessageBox( IDS_PROMPT_REGENERATE_ALBUM, MB_OKCANCEL | MB_ICONQUESTION ) )
				RegenerateModel( FM_Regeneration );
	}
}

void CAlbumDoc::OnUpdateArchiveImages( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( HasImages() );
}

void CAlbumDoc::OnEditArchivePassword( void )
{
	fs::CPath docFilePath( GetPathName().GetString() );
	CPasswordDialog dlg( NULL, &docFilePath );
	dlg.SetPassword( m_password );

	if ( IDOK == dlg.DoModal() )
	{
		m_password = dlg.GetPassword();

		if ( IsStorageAlbum() )
			if ( ICatalogStorage* pCatalogStorage = GetCatalogStorage() )
			{
				pCatalogStorage->StorePassword( m_password );
				CCatalogPasswordStore::Instance()->SavePassword( pCatalogStorage );
			}
	}
}

void CAlbumDoc::OnUpdateEditArchivePassword( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();		// enable editing for any document (including .sld), in preparation for SaveAs .ias

	if ( CButton* pButton = (CButton*)pCmdUI->m_pOther )		// check-box button in album settings dialog?
		CIconButton::SetButtonIcon( pButton, CIconId( !m_password.empty() ? pCmdUI->m_nID : 0 ) );
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
	if ( CAlbumImageView* pImageView = GetAlbumImageView() )
		pImageView->GetPeerThumbView()->SelectAll();
}

void CAlbumDoc::OnUpdateSelectAllThumbs( CCmdUI* pCmdUI )
{
	CAlbumImageView* pImageView = GetAlbumImageView();
	pCmdUI->Enable( pImageView != NULL && HasFlag( pImageView->GetSlideData().m_viewFlags, af::ShowThumbView ) );
}
