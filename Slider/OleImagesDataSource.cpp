
#include "stdafx.h"
#include "OleImagesDataSource.h"
#include "FileOperation.h"
#include "Workspace.h"
#include "Application.h"
#include "utl/FileEnumerator.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/WicDibSection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ole
{
	CImagesDataSource::~CImagesDataSource()
	{
	}

	void CImagesDataSource::CacheShellFilePaths( const std::vector< fs::CPath >& filePaths )
	{
		if ( !m_tempClones.SetInputFilePaths( filePaths ) )			// use temporary copies of embedded image files
			return;

		CDataSource::CacheShellFilePaths( m_tempClones.GetPhysicalFilePaths() );
	}

	DROPEFFECT CImagesDataSource::DragAndDropImages( HWND hSrcWnd, DROPEFFECT dropEffect, const RECT* pStartDragRect /*= NULL*/ )
	{
		CScopedDisableDropTarget disableDropTarget( AfxGetMainWnd() );

		GetDragImager().SetFromWindow( hSrcWnd );			// will send a DI_GETDRAGIMAGE message to pSrcWnd
		return DragAndDrop( dropEffect, pStartDragRect );
	}

	DROPEFFECT CImagesDataSource::DragAndDropImages( CWicDibSection* pBitmap, DROPEFFECT dropEffect, const RECT* pStartDragRect /*= NULL*/ )
	{
		CScopedDisableDropTarget disableDropTarget( AfxGetMainWnd() );

		if ( pBitmap != NULL )
			GetDragImager().SetFromBitmap( pBitmap->CloneBitmap(), CLR_NONE );		// drag imager will take ownership of the bitmap, so we need to pass a clone

		return DragAndDrop( dropEffect, pStartDragRect );
	}
}


// CTempCloneFileSet implementation

CTempCloneFileSet::~CTempCloneFileSet()
{
	DeleteClones();
}

bool CTempCloneFileSet::SetInputFilePaths( const std::vector< fs::CFlexPath >& inputFilePaths )
{
	ClearAllTempFiles();			// delete all previously cloned files to avoid renames due to collisions

///	std::auto_ptr< app::CScopedProgress > pProgress;

///	if ( HasFlag( CWorkspace::GetFlags(), wf::AllowEmbeddedFileTransfers ) )
///		pProgress.reset( new app::CScopedProgress( 0, (int)inputFilePaths.size(), 1, _T("Create physical backup:") ) );

	const fs::CPath& tempDirPath = GetTempDirPath();

	ASSERT( tempDirPath.FileExist() );
	for ( std::vector< fs::CFlexPath >::const_iterator itInputPath = inputFilePaths.begin(); itInputPath != inputFilePaths.end(); ++itInputPath )
	{
		if ( itInputPath->FileExist() )
		{
			if ( itInputPath->IsComplexPath() )
			{
				if ( HasFlag( CWorkspace::GetFlags(), wf::AllowEmbeddedFileTransfers ) )
				{
					// make a unique numeric physical temp path
					fs::CFlexPath physicalTempPath( fs::MakeUniqueNumFilename( tempDirPath / itInputPath->GetFilename() ).Get() );

					CFileOperation fileOperation;
					if ( fileOperation.Copy( *itInputPath, physicalTempPath ) )
					{
						m_tempClonedImagePaths.push_back( std::make_pair( *itInputPath, physicalTempPath ) );		// store the logical to physical pair for further destruction
						m_physicalFilePaths.push_back( physicalTempPath );							// add the physical replacement for the logical input file
					}
					else
						TRACE( _T("Cannot clone archived image %s to file %s\n"), itInputPath->GetPtr(), physicalTempPath.GetPtr() );
				}
			}
			else
				m_physicalFilePaths.push_back( *itInputPath );
		}

///		if ( pProgress.get() != NULL )
///			pProgress->StepIt();
	}

	return !m_physicalFilePaths.empty();
}

void CTempCloneFileSet::DeleteClones( void )
{
	const fs::CPath& tempDirPath = GetTempDirPath();
	if ( !fs::IsValidDirectory( tempDirPath.GetPtr() ) )
		return;

	for ( std::vector< std::pair< fs::CFlexPath, fs::CPath > >::const_iterator itLogical = m_tempClonedImagePaths.begin(); itLogical != m_tempClonedImagePaths.end(); ++itLogical )
		if ( itLogical->second.FileExist() )
			try
			{
				CFile::Remove( itLogical->second.GetPtr() );
			}
			catch ( CException* pExc )
			{
				app::HandleException( pExc );
			}

	m_physicalFilePaths.clear();
	m_tempClonedImagePaths.clear();
}

const fs::CPath& CTempCloneFileSet::GetTempDirPath( void )
{
	static const fs::CPath s_cloneSubDir( _T("SliderTempClones") );
	static fs::CPath dirPath;

	if ( dirPath.IsEmpty() )
	{
		dirPath = fs::GetTempDirPath() / s_cloneSubDir;
		if ( !fs::IsValidDirectory( dirPath.GetPtr() ) )
			try
			{
				fs::thr::CreateDirPath( dirPath.GetPtr(), fs::MfcExc );
			}
			catch ( CException* pExc )
			{
				app::HandleException( pExc );
			}
	}

	return dirPath;
}

bool CTempCloneFileSet::ClearAllTempFiles( void )
{
	return fs::DeleteAllFiles( GetTempDirPath().GetPtr() );
}
