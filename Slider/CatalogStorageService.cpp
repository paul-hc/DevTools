
#include "stdafx.h"
#include "CatalogStorageService.h"
#include "AlbumDoc.h"
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


// CCatalogStorageService implementation

const TCHAR CCatalogStorageService::s_buildTag[] = _T("Building image file attributes");

CCatalogStorageService::CCatalogStorageService( void )
	: m_pProgressSvc( ui::CNoProgressService::Instance() )
	, m_pUserReport( &ui::CSilentMode::Instance() )
	, m_pAlbumDoc( NULL )
	, m_isManagedAlbum( false )
{
}

CCatalogStorageService::CCatalogStorageService( ui::IProgressService* pProgressSvc, ui::IUserReport* pUserReport )
	: m_pProgressSvc( pProgressSvc )
	, m_pUserReport( pUserReport )
	, m_pAlbumDoc( NULL )
	, m_isManagedAlbum( false )
{
	ASSERT_PTR( m_pProgressSvc );		// using null-pattern
	ASSERT_PTR( m_pUserReport );
}

CCatalogStorageService::~CCatalogStorageService()
{
	utl::ClearOwningContainer( m_transferAttrs );

	if ( m_isManagedAlbum )
		delete m_pAlbumDoc;
}

void CCatalogStorageService::BuildFromAlbumSaveAs( const CAlbumDoc* pSrcAlbumDoc )
{
	const CAlbumModel* pSrcModel = pSrcAlbumDoc->GetModel();
	bool useDeepStreamPaths = pSrcModel->HasPersistFlag( CAlbumModel::UseDeepStreamPaths ) || CAlbumModel::ShouldUseDeepStreamPaths();

	BuildTransferAttrs( &pSrcModel->GetImagesModel(), useDeepStreamPaths );
	CloneDestAlbumDoc( pSrcAlbumDoc );
}

void CCatalogStorageService::BuildTransferAttrs( const CImagesModel* pImagesModel, bool useDeepStreamPaths /*= true*/ )
{
	ASSERT_PTR( pImagesModel );
	REQUIRE( m_transferAttrs.empty() && m_srcDocStgPaths.empty() && m_srcStorageHost.IsEmpty() );

	const std::vector< CFileAttr* >& srcFileAttrs = pImagesModel->GetFileAttrs();

	m_pProgressSvc->AdvanceStage( s_buildTag );
	m_pProgressSvc->SetBoundedProgressCount( srcFileAttrs.size() );

	std::vector< fs::TEmbeddedPath > destImagePaths;
	utl::Assign( destImagePaths, srcFileAttrs, func::ToFilePath() );
	fattr::TransformDestEmbeddedPaths( destImagePaths, useDeepStreamPaths );
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
	fattr::MakeTransferPathPairs( xferPairs, srcImagePaths, true );

	BuildFromTransferPairs( xferPairs );

	// create an ad-hoc album based on the transfer attributes, to save to "_Album.sld" stream
	REQUIRE( NULL == m_pAlbumDoc && !m_isManagedAlbum );

	m_isManagedAlbum = true;
	m_pAlbumDoc = RUNTIME_CLASS( CAlbumDoc )->CreateObject();

	CAlbumDoc* pAlbumDoc = checked_static_cast< CAlbumDoc* >( m_pAlbumDoc );
	CImagesModel* pImagesModel = &pAlbumDoc->RefModel()->RefImagesModel();

	for ( std::vector< CTransferFileAttr* >::const_iterator itTransferAttr = m_transferAttrs.begin(); itTransferAttr != m_transferAttrs.end(); ++itTransferAttr )
		pImagesModel->AddFileAttr( new CFileAttr( **itTransferAttr ) );

	if ( !IsEmpty() )
		pAlbumDoc->m_slideData.SetCurrentIndex( 0 );
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
				utl::AddUnique( m_srcDocStgPaths, itPair->first.GetPhysicalPath() );	// storage file to open for reading
		}
		else
		{
			CFileException error( CFileException::fileNotFound, -1, itPair->first.GetPtr() );		// source image doesn't exist
			m_pUserReport->ReportError( &error, MB_OK | MB_ICONWARNING );							// just warn & keep going
		}

	fattr::StoreBaselineSequence( m_transferAttrs );		// reset baseline to existing order

	m_srcStorageHost.PushMultiple( m_srcDocStgPaths );		// open source storages for reading
}


CAlbumDoc* CCatalogStorageService::CloneDestAlbumDoc( const CAlbumDoc* pSrcAlbumDoc )
{
	ASSERT_PTR( pSrcAlbumDoc );
	REQUIRE( NULL == m_pAlbumDoc && !m_isManagedAlbum );
	REQUIRE( !m_transferAttrs.empty() );

	m_pAlbumDoc = RUNTIME_CLASS( CAlbumDoc )->CreateObject();		// new temporary DEST album to build and SaveAs
	m_isManagedAlbum = true;

	CAlbumDoc* pDestAlbumDoc = checked_static_cast< CAlbumDoc* >( m_pAlbumDoc );

	m_password = pSrcAlbumDoc->m_password;
	pDestAlbumDoc->CopyAlbumState( pSrcAlbumDoc );		// copy album state (slide data, background color, etc)

	CAlbumModel* pDestModel = pDestAlbumDoc->RefModel();
	pDestModel->SetPersistFlag( CAlbumModel::UseDeepStreamPaths, CAlbumModel::ShouldUseDeepStreamPaths() );			// keep track of saving structure

	CImagesModel* pDestImagesModel = &pDestModel->RefImagesModel();

	// copy the transfer attributes as album attributes
	for ( std::vector< CTransferFileAttr* >::const_iterator itTransferAttr = m_transferAttrs.begin(); itTransferAttr != m_transferAttrs.end(); ++itTransferAttr )
		pDestImagesModel->AddFileAttr( new CFileAttr( **itTransferAttr ) );

	return pDestAlbumDoc;
}
