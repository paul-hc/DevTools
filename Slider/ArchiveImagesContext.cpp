
#include "StdAfx.h"
#include "ArchiveImagesContext.h"
#include "ImageArchiveStg.h"
#include "FileList.h"
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
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CEnumTags& GetTags_FileOp( void )
{
	static const CEnumTags tags( _T("File Group Copy|File Group Move|Custom Order") );
	return tags;
}


// CArchiveImagesContext implementation

CArchiveImagesContext::CArchiveImagesContext( void )
{
}

CArchiveImagesContext::~CArchiveImagesContext()
{
}

void CArchiveImagesContext::Stream( CArchive& archive )
{
	serial::SerializeValues( archive, m_pathPairs );
}

bool CArchiveImagesContext::CreateArchiveStgFile( CFileList* pFileList, const fs::CPath& destStgPath, CObject* pAlbumDoc )
{
	std::vector< CFileAttr* > srcFiles;
	pFileList->QueryFileAttrs( srcFiles );
	SetupSourcePaths( srcFiles );

	UINT seqCount = 0;
	static const std::tstring formatCopySrc = _T("*.*");
	GenerateDestPaths( destStgPath, formatCopySrc, &seqCount );							// make m_pathPairs: copy SRC as is into DEST

	pFileList->CheckReparentFileAttrs( destStgPath.GetPtr(), CFileList::Saving );		// reparent with destStgPath before saving the album info

	return BuildArchiveStgFile( destStgPath, FOP_FileCopy, pAlbumDoc );					// false: user declined overwrite
}

void CArchiveImagesContext::SetupSourcePaths( const std::vector< CFileAttr* >& srcFiles )
{
	m_pathPairs.clear();
	m_pathPairs.resize( srcFiles.size() );

	for ( size_t i = 0; i != srcFiles.size(); ++i )
		m_pathPairs[ i ].first = srcFiles[ i ]->GetPath();
}

void CArchiveImagesContext::ResetDestPaths( void )
{
	for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
		it->second.Clear();			// reset the destination filepath
}

bool CArchiveImagesContext::CommitOperations( FileOp fileOp, bool isUndoOp /*= false*/ ) const
{
	CWaitCursor wait;

	for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); )
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

void CArchiveImagesContext::CommitOperation( FileOp fileOp, const std::pair< fs::CFlexPath, fs::CFlexPath >& filePair, bool isUndoOp /*= false*/ ) throws_( CException* )
{
	CFileOperation fileOperation( true );
	bool succeeded = false;

	switch ( fileOp )
	{
		case FOP_FileCopy:
			if ( !isUndoOp )
				succeeded = fileOperation.Copy( filePair.first, filePair.second );		// copy SRC to DEST
			else
				succeeded = fileOperation.Delete( filePair.second );					// delete DEST
			break;
		case FOP_FileMove:
			if ( !isUndoOp )
				succeeded = fileOperation.Move( filePair.first, filePair.second );		// move SRC to DEST
			else
				succeeded = fileOperation.Move( filePair.second, filePair.first );		// move DEST to SRC
			break;
		default:
			ASSERT( false );
	}

	fileOperation.Log( app::GetApp()->GetLogger() );
}

bool CArchiveImagesContext::CanCommitOperations( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& rErrorPairs, FileOp fileOp, bool isUndoOp ) const
{
	rErrorPairs.clear();

	for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
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

bool CArchiveImagesContext::IsValidFormat( const std::tstring& format )
{
	CPathFormatter formatter( format );
	return formatter.IsValidFormat();
}

bool CArchiveImagesContext::GenerateDestPaths( const fs::CPath& destPath, const std::tstring& format, UINT* pSeqCount, bool forceShallowStreamNames /*= false*/ )
{
	ASSERT_PTR( pSeqCount );
	ResetDestPaths();

	DestType destType = app::IsImageArchiveDoc( destPath.GetPtr() ) ? ToArchiveStg : ToDirectory;
	if ( ToDirectory == destType && !destPath.FileExist() )
		return false;
	if ( m_pathPairs.empty() )
		return false;

	CPathGenerator generator( format, *pSeqCount );
	generator.StoreSrcFromPairs( m_pathPairs );							// copy source paths from vector to map

	switch ( destType )
	{
		case ToDirectory:	generator.SetMoveDestDirPath( destPath.GetDirPath( true ) ); break;
		case ToArchiveStg:	generator.SetMoveDestDirPath( std::tstring() ); break;					// flat embedded streams use no dir path
		default: ASSERT( false );
	}

	bool generated = false;
	if ( ToArchiveStg == destType )
		if ( HasFlag( CWorkspace::GetFlags(), wf::PrefixDeepStreamNames ) && !forceShallowStreamNames )
			if ( generator.MakeDestStripCommonPrefix() )					// prefix with subpaths from the common prefix of SRC image paths
			{
				generated = true;
				generator.ForEachDestPath( func::StripStgDocPrefix() );		// clear stg doc prefix as it may be part of the common prefix
			}

	if ( !generated )
		if ( !generator.GeneratePairs() )
			return false;

	generator.QueryDestToPairs( m_pathPairs );							// copy dest paths from generated map to vector
	*pSeqCount = generator.GetSeqCount();
	return true;
}

bool CArchiveImagesContext::BuildArchiveStgFile( const fs::CPath& destStgPath, FileOp fileOp, CObject* pAlbumDoc /*= NULL*/ ) const
{
	try
	{
		app::LogLine( _T("--- START building image archive %s ---"), destStgPath.GetPtr() );

		CTimer timer;
		CImageArchiveStg imageArchiveStg;
		imageArchiveStg.CreateImageArchive( destStgPath.GetPtr(), m_password, m_pathPairs, pAlbumDoc );

		app::LogLine( _T("--- END building image archive %s - Elapsed %.2f seconds ---"), destStgPath.GetPtr(), timer.ElapsedSeconds() );
	}
	catch ( mfc::CUserAbortedException* pAbortExc )
	{	// aborted by user, don't prompt again
		pAbortExc;
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
		app::CScopedProgress progress( 0, (int)m_pathPairs.size(), 1, _T("Delete source images:") );

		for ( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); )
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

			++it;
			progress.StepIt();
		}
	}
	return 0 == errorCount;
}