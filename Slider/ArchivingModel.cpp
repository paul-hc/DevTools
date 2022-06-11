
#include "stdafx.h"
#include "ArchivingModel.h"
#include "CatalogStorageService.h"
#include "ICatalogStorage.h"
#include "AlbumModel.h"
#include "FileAttr.h"
#include "FileAttrAlgorithms.h"
#include "FileOperation.h"
#include "Workspace.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/Logger.h"
#include "utl/RuntimeException.h"
#include "utl/PathGenerator.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StructuredStorage.h"
#include "utl/Timer.h"
#include "utl/UI/ProgressService.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CEnumTags& GetTags_FileOp( void )
{
	static const CEnumTags tags( _T("File Group Copy|File Group Move|Custom Order") );
	return tags;
}


// CArchivingModel implementation

CArchivingModel::CArchivingModel( void )
{
}

CArchivingModel::~CArchivingModel()
{
}

void CArchivingModel::Stream( CArchive& archive )
{
	serial::SerializeValues( archive, m_pathPairs );
}

void CArchivingModel::SetupSourcePaths( const std::vector< CFileAttr* >& srcFiles )
{
	m_pathPairs.clear();
	m_pathPairs.resize( srcFiles.size() );

	for ( size_t i = 0; i != srcFiles.size(); ++i )
		m_pathPairs[ i ].first = srcFiles[ i ]->GetPath();
}

void CArchivingModel::ResetDestPaths( void )
{
	for ( std::vector< TTransferPathPair >::iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
		it->second.Clear();			// reset the destination filepath
}

bool CArchivingModel::CommitOperations( FileOp fileOp, bool isUndoOp /*= false*/ ) const
{
	CWaitCursor wait;

	for ( std::vector< TTransferPathPair >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); )
	{
		try
		{
			CommitOperation( fileOp, *it, isUndoOp );
		}
		catch ( CException* pExc )
		{
			switch ( app::GetUserReport().ReportError( pExc, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
			{
				case IDABORT:	return false;
				case IDRETRY:	continue;
				case IDIGNORE:	break;
			}
		}
		++it;
	}

	return true;
}

void CArchivingModel::CommitOperation( FileOp fileOp, const TTransferPathPair& xferPair, bool isUndoOp /*= false*/ ) throws_( CException* )
{
	CFileOperation fileOperation( utl::ThrowMode );
	bool succeeded = false;

	switch ( fileOp )
	{
		case FOP_FileCopy:
			if ( !isUndoOp )
				succeeded = fileOperation.Copy( xferPair.first, xferPair.second );		// copy SRC to DEST
			else
				succeeded = fileOperation.Delete( xferPair.second );					// delete DEST
			break;
		case FOP_FileMove:
			if ( !isUndoOp )
				succeeded = fileOperation.Move( xferPair.first, xferPair.second );		// move SRC to DEST
			else
				succeeded = fileOperation.Move( xferPair.second, xferPair.first );		// move DEST to SRC
			break;
		default:
			ASSERT( false );
	}

	fileOperation.Log( app::GetApp()->GetLogger() );
}

bool CArchivingModel::CanCommitOperations( std::vector< TTransferPathPair >& rErrorPairs, FileOp fileOp, bool isUndoOp ) const
{
	rErrorPairs.clear();

	for ( std::vector< TTransferPathPair >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
		switch ( fileOp )
		{
			case FOP_FileCopy:
				if ( !isUndoOp )
				{	// copy SRC to DEST
					if ( !it->first.FileExist() || it->second.FileExist() )
						rErrorPairs.push_back( *it );		// no SRC or DEST already exists
				}
				else
					// delete DEST
					if ( !it->first.FileExist() || !it->second.FileExist() )
						rErrorPairs.push_back( *it );		// no SRC or no DEST
				break;
			case FOP_FileMove:
				if ( !isUndoOp )
				{	// move SRC to DEST
					if ( !it->first.FileExist() || it->second.FileExist() )
						rErrorPairs.push_back( *it );		// no SRC or DEST already exists
				}
				else
					// move DEST to SRC
					if ( it->first.FileExist() || !it->second.FileExist() )
						rErrorPairs.push_back( *it );		// SRC already exists or no DEST
				break;
			default:
				ASSERT( false );
		}

	return rErrorPairs.empty();		// true if no errors
}

bool CArchivingModel::IsValidFormat( const std::tstring& format )
{
	CPathFormatter formatter( format, false );
	return formatter.IsValidFormat();
}

bool CArchivingModel::GenerateDestPaths( const fs::CPath& destPath, const std::tstring& format, UINT* pSeqCount, bool forceShallowStreamNames /*= false*/ )
{
	ASSERT_PTR( pSeqCount );
	ResetDestPaths();

	DestType destType = app::IsCatalogFile( destPath.GetPtr() ) ? ToArchiveStg : ToDirectory;
	if ( ToDirectory == destType && !destPath.FileExist() )
		return false;
	if ( m_pathPairs.empty() )
		return false;

	CPathFormatter formatter( format, false );
	CPathGenerator generator( formatter, *pSeqCount );
	generator.RefRenamePairs()->StoreSrcFromPairs( m_pathPairs );			// copy source paths from vector to map

	switch ( destType )
	{
		case ToDirectory:	generator.SetMoveDestDirPath( destPath.GetParentPath( true ) ); break;
		case ToArchiveStg:	generator.SetMoveDestDirPath( fs::CPath() ); break;					// flat embedded streams use no dir path
		default: ASSERT( false );
	}

	bool generated = false;
	if ( ToArchiveStg == destType )
		if ( HasFlag( CWorkspace::GetFlags(), wf::DeepStreamPaths ) && !forceShallowStreamNames )
			if ( generator.MakeDestStripCommonPrefix() )					// prefix with subpaths from the common prefix of SRC image paths
			{
				generated = true;

				generator.RefRenamePairs()->ForEachDestPath( func::NormalizeComplexPath() );	// convert any deep embedded storage paths to directory paths
			}

	if ( !generated )
		if ( !generator.GeneratePairs() )
			return false;

	generator.GetRenamePairs()->QueryDestToPairs( m_pathPairs );							// copy dest paths from generated map to vector
	*pSeqCount = generator.GetSeqCount();
	return true;
}

bool CArchivingModel::BuildArchiveStorageFile( const fs::TStgDocPath& destStgPath, FileOp fileOp, CWnd* pParentWnd /*= AfxGetMainWnd()*/ ) const
{
	// For now disable interactive progress since if current image is animated, it will acces its shared decoder, and the stream will be temporarily inaccessible.
	// Still, error message boxes are still problematic... that should be non-interfactive too.
	//
		//pParentWnd;
	CProgressService progress( pParentWnd, _T("Building image archive storage file") );
	utl::IProgressService* pProgressSvc = /*svc::CNoProgressService::Instance()*/ progress.GetService();

	CCatalogStorageService catalogSvc( pProgressSvc, &app::GetUserReport() );		// image storage metadata - owning for exception safety

	catalogSvc.SetPassword( m_password );
	catalogSvc.BuildFromTransferPairs( m_pathPairs );		// also discards cached SRC images and storages

	if ( catalogSvc.IsEmpty() )
		if ( catalogSvc.GetReport()->MessageBox( _T("There are no files to add to the image archive.\nDo you still want to create an empty image archive?"), MB_YESNO | MB_ICONQUESTION ) != IDYES )
			return false;

	try
	{
		app::LogLine( _T("--- START building image archive %s ---"), destStgPath.GetPtr() );

		CTimer timer;

		CComPtr<ICatalogStorage> pCatalogStorage = CCatalogStorageFactory::CreateStorageObject();

		pCatalogStorage->CreateImageArchiveFile( destStgPath, &catalogSvc );

		app::LogLine( _T("--- END building image archive %s - Elapsed %.2f seconds ---"), destStgPath.GetPtr(), timer.ElapsedSeconds() );
	}
	catch ( CUserAbortedException& exc )
	{
		app::TraceException( exc );						// cancelled by the user in progress dialog
		::DeleteFile( destStgPath.GetPtr() );			// on exception delete archive file
		return false;
	}
	catch ( mfc::CUserAbortedException* pAbortExc )
	{	// aborted by user, don't prompt again
		app::TraceException( pAbortExc );				// cancelled by the user: keep the images found so far
		::DeleteFile( destStgPath.GetPtr() );			// on exception delete archive file
		return false;
	}
	catch ( CException* pExc )
	{
		app::GetUserReport().ReportError( pExc, MB_OK | MB_ICONEXCLAMATION );
		::DeleteFile( destStgPath.GetPtr() );			// on exception delete archive file
		return false;
	}

	// successful MOVE operation: delete the source files
	size_t errorCount = 0;
	if ( FOP_FileMove == fileOp )
	{
		pProgressSvc->AdvanceStage( _T("Delete source image files") );
		pProgressSvc->SetBoundedProgressCount( m_pathPairs.size() );

		for ( std::vector< TTransferPathPair >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); )
		{
			if ( it->first.FileExist() )
				if ( !it->first.IsComplexPath() )
					try
					{
						CFile::Remove( it->first.GetPtr() );
					}
					catch ( CException* pExc )
					{
						++errorCount;
						switch ( app::GetUserReport().ReportError( pExc, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION ) )
						{
							case IDABORT:	return false;
							case IDRETRY:	continue;
							case IDIGNORE:	break;
						}
					}

			pProgressSvc->AdvanceItem( it->second.Get() );
			++it;
		}
	}

	return 0 == errorCount;
}
