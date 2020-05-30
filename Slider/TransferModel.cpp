
#include "stdafx.h"
#include "TransferModel.h"
#include "utl/ContainerUtilities.h"
#include "utl/UI/IProgressService.h"
#include "utl/UI/UserReport.h"
#include "utl/UI/WicImageCache.h"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTransferModel::~CTransferModel()
{
	utl::ClearOwningContainer( m_fileAttribs );
}

void CTransferModel::Build( const std::vector< TTransferPathPair >& xferPairs, ui::IProgressService* pProgressService, ui::IUserReport* pUserReport )
{
	ASSERT_PTR( pProgressService );
	ASSERT_PTR( pUserReport );

	pProgressService->AdvanceStage( _T("Building image file attributes") );
	pProgressService->SetBoundedProgressCount( xferPairs.size() );

	stdext::hash_set< fs::CPath > srcStorages;

	m_fileAttribs.reserve( xferPairs.size() );

	for ( std::vector< TTransferPathPair >::const_iterator itPair = xferPairs.begin(); itPair != xferPairs.end(); ++itPair )
		if ( itPair->first.FileExist() )
		{
			m_fileAttribs.push_back( new CTransferFileAttr( *itPair ) );

			if ( itPair->first.IsComplexPath() )
				srcStorages.insert( itPair->first.GetPhysicalPath() );			// storage file to discard from cache at end
			else
				CWicImageCache::Instance().DiscardFrames( itPair->first );		// done with the image, discard it from cache
		}
		else
		{
			CFileException error( CFileException::fileNotFound, -1, itPair->first.GetPtr() );		// source image doesn't exist
			pUserReport->ReportError( &error, MB_OK | MB_ICONWARNING );								// just warn & keep going
		}

	// Prevent sharing violations on SRC stream open.
	//	2020-04-11: Still doesn't work, I get exception on open. I suspect the source stream (image file) must be kept open with CFile::shareExclusive by some WIC indirect COM interface.

	for ( stdext::hash_set< fs::CPath >::const_iterator itSrcStorage = srcStorages.begin(); itSrcStorage != srcStorages.end(); ++itSrcStorage )
		CWicImageCache::Instance().DiscardWithPrefix( itSrcStorage->GetPtr() );		// discard cached images for the storage
}
