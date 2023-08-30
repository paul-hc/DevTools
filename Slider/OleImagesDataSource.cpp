
#include "pch.h"
#include "OleImagesDataSource.h"
#include "FileOperation.h"
#include "Workspace.h"
#include "Application_fwd.h"
#include "utl/Algorithms.h"
#include "utl/FileEnumerator.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/StatusProgressService.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/WicDibSection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ole
{
	CImagesDataSource::~CImagesDataSource()
	{
	}

	void CImagesDataSource::CacheShellFilePaths( const std::vector<fs::CPath>& filePaths )
	{
		if ( !m_tempClones.SetInputFilePaths( filePaths ) )			// use temporary copies of embedded image files
			return;

		CDataSource::CacheShellFilePaths( m_tempClones.GetPhysicalFilePaths() );
	}

	DROPEFFECT CImagesDataSource::DragAndDropImages( HWND hSrcWnd, DROPEFFECT dropEffect, const RECT* pStartDragRect /*= nullptr*/ )
	{
		CScopedDisableDropTarget disableDropTarget( AfxGetMainWnd() );

		GetDragImager().SetFromWindow( hSrcWnd );			// will send a DI_GETDRAGIMAGE message to pSrcWnd
		return DragAndDrop( dropEffect, pStartDragRect );
	}

	DROPEFFECT CImagesDataSource::DragAndDropImages( CWicDibSection* pBitmap, DROPEFFECT dropEffect, const RECT* pStartDragRect /*= nullptr*/ )
	{
		CScopedDisableDropTarget disableDropTarget( AfxGetMainWnd() );

		if ( pBitmap != nullptr )
			GetDragImager().SetFromBitmap( pBitmap->CloneBitmap(), CLR_NONE );		// drag imager will take ownership of the bitmap, so we need to pass a clone

		return DragAndDrop( dropEffect, pStartDragRect );
	}
}


// CTempCloneFileSet implementation

CTempCloneFileSet::~CTempCloneFileSet()
{
	DeleteClones();
}

template< typename SrcContainerT >
bool CTempCloneFileSet::SetInputFilePaths( const SrcContainerT& inputFilePaths )
{
	std::vector<fs::CFlexPath> flexPaths;
	utl::Assign( flexPaths, inputFilePaths, func::tor::StringOf() );
	return SetInputFilePaths( flexPaths );
}

bool CTempCloneFileSet::SetInputFilePaths( const std::vector<fs::CFlexPath>& inputFilePaths )
{
	ClearAllTempFiles();			// delete all previously cloned files to avoid renames due to collisions

	const fs::TDirPath& tempDirPath = GetTempDirPath();
	CStatusProgressService progressSvc;

	ASSERT( tempDirPath.FileExist() );
	for ( std::vector<fs::CFlexPath>::const_iterator itInputPath = inputFilePaths.begin(); itInputPath != inputFilePaths.end(); ++itInputPath )
	{
		if ( itInputPath->FileExist() )
		{
			if ( itInputPath->IsComplexPath() )
			{	// file drag & drop works only with physical files: we have to mirror the storage files as temporary physical copies during drag & drop
				if ( HasFlag( CWorkspace::GetFlags(), wf::AllowEmbeddedFileTransfers ) )
				{
					if ( !progressSvc.IsActive() )
					{	// lazy enter progress mode
						progressSvc.StartProgress( inputFilePaths.size() );
						progressSvc.SetLabelText( _T("Create physical backup:") );
					}

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

		if ( progressSvc.IsActive() )
			progressSvc.Advance();
	}

	return !m_physicalFilePaths.empty();
}

void CTempCloneFileSet::DeleteClones( void )
{
	const fs::TDirPath& tempDirPath = GetTempDirPath();
	if ( !fs::IsValidDirectory( tempDirPath.GetPtr() ) )
		return;

	for ( std::vector< std::pair<fs::CFlexPath, fs::CPath> >::const_iterator itLogical = m_tempClonedImagePaths.begin(); itLogical != m_tempClonedImagePaths.end(); ++itLogical )
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

const fs::TDirPath& CTempCloneFileSet::GetTempDirPath( void )
{
	static const TCHAR s_cloneSubDir[] =_T("SliderTempClones");
	static fs::TDirPath s_dirPath;

	if ( s_dirPath.IsEmpty() )
	{
		s_dirPath = fs::GetTempDirPath() / s_cloneSubDir;

		if ( !fs::IsValidDirectory( s_dirPath.GetPtr() ) )
			try
			{
				fs::thr::CreateDirPath( s_dirPath.GetPtr(), fs::MfcExc );
			}
			catch ( CException* pExc )
			{
				app::HandleException( pExc );
			}
	}

	return s_dirPath;
}

bool CTempCloneFileSet::ClearAllTempFiles( void )
{
	return fs::DeleteAllFiles( GetTempDirPath().GetPtr() );
}
