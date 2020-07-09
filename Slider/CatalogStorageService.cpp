
#include "stdafx.h"
#include "CatalogStorageService.h"
#include "FileAttrAlgorithms.h"
#include "AlbumDoc.h"
#include "utl/ContainerUtilities.h"
#include "utl/PathUniqueMaker.h"
#include "utl/UI/IProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace svc
{
	CAlbumModel* ToAlbumModel( CObject* pAlbumDoc )
	{
		CAlbumDoc* pDestAlbumDoc = checked_static_cast< CAlbumDoc* >( pAlbumDoc );
		ASSERT_PTR( pDestAlbumDoc );
		return pDestAlbumDoc->RefModel();
	}

	CImagesModel& ToImagesModel( CObject* pAlbumDoc )
	{
		return ToAlbumModel( pAlbumDoc )->RefImagesModel();
	}
}


// CTransferFileAttr implementation

CTransferFileAttr::CTransferFileAttr( const TTransferPathPair& pathPair )
	: CFileAttr( pathPair.first )		// inherit SRC file attribute
	, m_srcImagePath( pathPair.first )
{
	GetImageDim();						// cache dimensions via image loading
	SetPathKey( fs::ImagePathKey( pathPair.second, 0 ) );		// change to DEST path for transfer
}

CTransferFileAttr::CTransferFileAttr( const CFileAttr& srcFileAttr, const fs::TEmbeddedPath& destImagePath )
	: CFileAttr( srcFileAttr )			// inherit all SRC
	, m_srcImagePath( GetPath() )
{
	GetImageDim();						// cache dimensions
	SetPathKey( fs::ImagePathKey( fs::CastFlexPath( destImagePath ), 0 ) );		// change to DEST path for transfer
}

CTransferFileAttr::~CTransferFileAttr()
{
}


// CTransferAlbumService implementation

const TCHAR CTransferAlbumService::s_buildTag[] = _T("Building image file attributes");

CTransferAlbumService::CTransferAlbumService( void )
	: m_pProgressSvc( ui::CNoProgressService::Instance() )
	, m_pUserReport( &ui::CSilentMode::Instance() )
{
}

CTransferAlbumService::CTransferAlbumService( ui::IProgressService* pProgressSvc, ui::IUserReport* pUserReport )
	: m_pProgressSvc( pProgressSvc )
	, m_pUserReport( pUserReport )
{
	ASSERT_PTR( m_pProgressSvc );		// using null-pattern
	ASSERT_PTR( m_pUserReport );
}

CTransferAlbumService::~CTransferAlbumService()
{
	utl::ClearOwningContainer( m_transferAttrs );
	m_pDestAlbumDoc.reset();
}

void CTransferAlbumService::BuildFromAlbumSaveAs( const CAlbumDoc* pSrcAlbumDoc )
{
	const CAlbumModel* pSrcModel = pSrcAlbumDoc->GetModel();
	bool useDeepStreamPaths = pSrcModel->HasPersistFlag( CAlbumModel::UseDeepStreamPaths ) || CAlbumModel::ShouldUseDeepStreamPaths();

	BuildTransferAttrs( &pSrcModel->GetImagesModel(), useDeepStreamPaths );
	CloneDestAlbumDoc( pSrcAlbumDoc );			// clone the source album in order to facilitate transfer from any .sld or .ias source album
}

void CTransferAlbumService::CloneDestAlbumDoc( const CAlbumDoc* pSrcAlbumDoc )
{
	ASSERT_PTR( pSrcAlbumDoc );

	if ( m_transferAttrs.empty() )
		TRACE( _T(" # Warning: creating an empty catalog storage for document '%s'\n"), pSrcAlbumDoc->GetDocFilePath().GetPtr() );

	m_pDestAlbumDoc.reset( checked_static_cast< CAlbumDoc* >( RUNTIME_CLASS( CAlbumDoc )->CreateObject() ) );	// new temporary DEST album to build and SaveAs

	m_password = pSrcAlbumDoc->m_password;
	m_pDestAlbumDoc->CopyAlbumState( pSrcAlbumDoc );		// copy album state (slide data, background color, etc)

	CAlbumModel* pDestModel = m_pDestAlbumDoc->RefModel();
	pDestModel->SetPersistFlag( CAlbumModel::UseDeepStreamPaths, CAlbumModel::ShouldUseDeepStreamPaths() );		// keep track of storage saving structure

	CImagesModel* pDestImagesModel = &pDestModel->RefImagesModel();

	// copy the transfer attributes as album attributes
	for ( std::vector< CTransferFileAttr* >::const_iterator itTransferAttr = m_transferAttrs.begin(); itTransferAttr != m_transferAttrs.end(); ++itTransferAttr )
		pDestImagesModel->AddFileAttr( new CFileAttr( **itTransferAttr ) );
}


namespace cvt
{
	// conversion to catalog storage .ias album

	size_t ConvertToEmbeddedPaths( std::vector< fs::TEmbeddedPath >& rDestStreamPaths, bool useDeepStreamPaths = true )
	{	// rDestStreamPaths: IN source paths, OUT embedded stream paths
		if ( useDeepStreamPaths )
		{
			fs::CPath commonDirPath = path::ExtractCommonParentPath( rDestStreamPaths );
			if ( !commonDirPath.IsEmpty() )
				path::StripDirPrefix( rDestStreamPaths, commonDirPath.GetPtr() );
			else
				path::StripRootPrefix( rDestStreamPaths );		// ignore drive letter

			// convert any deep embedded storage paths to directory paths (so that '>' appears only once in the final embedded path)
			utl::for_each( rDestStreamPaths, func::NormalizeComplexPath() );
		}
		else
			path::StripToFilename( rDestStreamPaths );			// will take care to resolve duplicate filenames

		CPathUniqueMaker uniqueMaker;
		return uniqueMaker.UniquifyPaths( rDestStreamPaths );	// returns the number of duplicate collisions (uniquified)
	}

	template< typename SrcPathT >
	void MakeTransferPathPairs( std::vector< TTransferPathPair >& rTransferPairs, const std::vector< SrcPathT >& srcImagePaths, bool useDeepStreamPaths = true )
	{
		std::vector< fs::TEmbeddedPath > destStreamPaths;
		utl::Assign( destStreamPaths, srcImagePaths, func::tor::StringOf() );

		ConvertToEmbeddedPaths( destStreamPaths, useDeepStreamPaths );
		ENSURE( srcImagePaths.size() == destStreamPaths.size() );

		rTransferPairs.clear();
		rTransferPairs.reserve( srcImagePaths.size() );

		for ( size_t i = 0; i != srcImagePaths.size(); ++i )
			rTransferPairs.push_back( TTransferPathPair( srcImagePaths[ i ].Get(), fs::CastFlexPath( destStreamPaths[ i ] ) ) );
	}
}


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


// CCatalogStorageService implementation

void CCatalogStorageService::BuildTransferAttrs( const CImagesModel* pImagesModel, bool useDeepStreamPaths /*= true*/ )
{
	ASSERT_PTR( pImagesModel );
	REQUIRE( m_transferAttrs.empty() && m_srcDocStgPaths.empty() && m_srcStorageHost.IsEmpty() );

	const std::vector< CFileAttr* >& srcFileAttrs = pImagesModel->GetFileAttrs();

	m_pProgressSvc->AdvanceStage( s_buildTag );
	m_pProgressSvc->SetBoundedProgressCount( srcFileAttrs.size() );

	std::vector< fs::TEmbeddedPath > destImagePaths;
	utl::Assign( destImagePaths, srcFileAttrs, func::ToFilePath() );
	cvt::ConvertToEmbeddedPaths( destImagePaths, useDeepStreamPaths );
	ENSURE( srcFileAttrs.size() == destImagePaths.size() );

	m_transferAttrs.reserve( srcFileAttrs.size() );

	for ( size_t i = 0; i != srcFileAttrs.size(); ++i )
	{
		const CFileAttr* pSrcFileAttr = srcFileAttrs[ i ];

		if ( pSrcFileAttr->IsValid() )
			m_transferAttrs.push_back( new CTransferFileAttr( *pSrcFileAttr, destImagePaths[ i ] ) );
		else
		{	// source image doesn't exist
			CFileException error( CFileException::fileNotFound, -1, pSrcFileAttr->GetPath().GetPtr() );
			m_pUserReport->ReportError( &error, MB_OK | MB_ICONWARNING );									// just warn & keep going
		}
	}

	m_srcDocStgPaths = pImagesModel->GetStoragePaths();
	m_srcStorageHost.PushMultiple( m_srcDocStgPaths );		// open source storages for reading
}

void CCatalogStorageService::BuildFromSrcPaths( const std::vector< fs::CPath >& srcImagePaths )
{
	std::vector< TTransferPathPair > xferPairs;
	cvt::MakeTransferPathPairs( xferPairs, srcImagePaths, true );

	BuildFromTransferPairs( xferPairs );

	// create an ad-hoc album based on the transfer attributes, to save to "_Album.sld" stream
	m_pDestAlbumDoc.reset( checked_static_cast< CAlbumDoc* >( RUNTIME_CLASS( CAlbumDoc )->CreateObject() ) );

	CImagesModel* pImagesModel = &m_pDestAlbumDoc->RefModel()->RefImagesModel();

	for ( std::vector< CTransferFileAttr* >::const_iterator itTransferAttr = m_transferAttrs.begin(); itTransferAttr != m_transferAttrs.end(); ++itTransferAttr )
		pImagesModel->AddFileAttr( new CFileAttr( **itTransferAttr ) );

	if ( !IsEmpty() )
		m_pDestAlbumDoc->m_slideData.SetCurrentIndex( 0 );
}

void CCatalogStorageService::BuildFromTransferPairs( const std::vector< TTransferPathPair >& xferPairs )
{
	m_pProgressSvc->AdvanceStage( s_buildTag );
	m_pProgressSvc->SetBoundedProgressCount( xferPairs.size() );

	m_transferAttrs.reserve( xferPairs.size() );

	for ( std::vector< TTransferPathPair >::const_iterator itPair = xferPairs.begin(); itPair != xferPairs.end(); ++itPair )
		if ( itPair->first.FileExist() )
		{
			m_transferAttrs.push_back( new CTransferFileAttr( *itPair ) );

			if ( itPair->first.IsComplexPath() )
				utl::AddUnique( m_srcDocStgPaths, itPair->first.GetPhysicalPath() );				// storage file to open for reading
		}
		else
		{
			CFileException error( CFileException::fileNotFound, -1, itPair->first.GetPtr() );		// source image doesn't exist
			m_pUserReport->ReportError( &error, MB_OK | MB_ICONWARNING );							// just warn & keep going
		}

	fattr::StoreBaselineSequence( m_transferAttrs );		// reset baseline to existing order

	m_srcStorageHost.PushMultiple( m_srcDocStgPaths );		// open source storages for reading
}
