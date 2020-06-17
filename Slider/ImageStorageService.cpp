
#include "stdafx.h"
#include "ImageStorageService.h"
#include "ImageArchiveStg.h"
#include "FileAttrAlgorithms.h"
#include "utl/ContainerUtilities.h"
#include "utl/UI/IProgressService.h"
#include "utl/UI/UserReport.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pwd
{
	template< typename CharType >
	inline void Crypt( CharType& rChr )
	{
		static const TCHAR charMask = _T('\xAD');
		rChr ^= charMask;
	}

	std::tstring ToEncrypted( const std::tstring& displayPassword )
	{
		std::tstring encryptedPassword = displayPassword;
		std::reverse( encryptedPassword.begin(), encryptedPassword.end() );

		for ( size_t i = 0; i != encryptedPassword.size(); ++i )
			Crypt( encryptedPassword[ i ] );

		return encryptedPassword;
	}

	std::tstring ToDecrypted( const std::tstring& encryptedPassword )
	{
		std::tstring displayPassword = encryptedPassword;
		for ( size_t i = 0; i != displayPassword.size(); ++i )
			Crypt( displayPassword[ i ] );

		std::reverse( displayPassword.begin(), displayPassword.end() );
		return displayPassword;
	}
}


// CImageStorageService implementation

CImageStorageService::CImageStorageService( void )
	: m_pProgressSvc( ui::CNoProgressService::Instance() )
	, m_pUserReport( &ui::CSilentMode::Instance() )
{
}

CImageStorageService::CImageStorageService( ui::IProgressService* pProgressSvc, ui::IUserReport* pUserReport )
	: m_pProgressSvc( pProgressSvc )
	, m_pUserReport( pUserReport )
{
	ASSERT_PTR( m_pProgressSvc );		// using null-pattern
	ASSERT_PTR( m_pUserReport );
}

CImageStorageService::~CImageStorageService()
{
	utl::ClearOwningContainer( m_transferAttrs );
}

void CImageStorageService::Build( const std::vector< TTransferPathPair >& xferPairs )
{
	m_pProgressSvc->AdvanceStage( _T("Building image file attributes") );
	m_pProgressSvc->SetBoundedProgressCount( xferPairs.size() );

	m_transferAttrs.reserve( xferPairs.size() );

	for ( std::vector< TTransferPathPair >::const_iterator itPair = xferPairs.begin(); itPair != xferPairs.end(); ++itPair )
		if ( itPair->first.FileExist() )
		{
			m_transferAttrs.push_back( new CTransferFileAttr( *itPair ) );

			if ( itPair->first.IsComplexPath() )
				utl::AddUnique( m_srcStorages, itPair->first.GetPhysicalPath() );	// storage file to discard from cache at end
			else
				CWicImageCache::Instance().DiscardFrames( itPair->first );			// done with the image, discard it from cache
		}
		else
		{
			CFileException error( CFileException::fileNotFound, -1, itPair->first.GetPtr() );		// source image doesn't exist
			m_pUserReport->ReportError( &error, MB_OK | MB_ICONWARNING );							// just warn & keep going
		}

	fattr::StoreBaselineSequence( m_transferAttrs );

	// Prevent sharing violations on SRC stream open.
	//	2020-04-11: Still doesn't work, I get exception on open. I suspect the source stream (image file) must be kept open with CFile::shareExclusive by some WIC indirect COM interface.

	for ( std::vector< fs::CPath >::const_iterator itSrcStorage = m_srcStorages.begin(); itSrcStorage != m_srcStorages.end(); ++itSrcStorage )
		CImageArchiveStg::DiscardCachedImages( *itSrcStorage );		// discard cached images and thumbs for the storage
}

void CImageStorageService::BuildFromSrcPaths( const std::vector< fs::CPath >& srcImagePaths )
{
	std::vector< TTransferPathPair > xferPairs;
	fattr::MakeTransferPathPairs( xferPairs, srcImagePaths, true );

	Build( xferPairs );
}

void CImageStorageService::MakeAlbumFileAttrs( std::vector< CFileAttr* >& rFileAttrs ) const
{
	utl::ClearOwningContainer( rFileAttrs );
	rFileAttrs.reserve( m_transferAttrs.size() );

	for ( std::vector< CTransferFileAttr* >::const_iterator itTransferAttr = m_transferAttrs.begin(); itTransferAttr != m_transferAttrs.end(); ++itTransferAttr )
		rFileAttrs.push_back( new CFileAttr( **itTransferAttr ) );
}
