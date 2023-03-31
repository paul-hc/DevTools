
#include "pch.h"
#include "MfcUtilities.h"
#include "Path.h"
#include "Serialization.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	CGlobalMemFile::CGlobalMemFile( HGLOBAL hSrcBuffer ) throws_( CException )
		: CMemFile( 0 )				// no growth when reading
		, m_hLockedSrcBuffer( nullptr )
	{
		if ( hSrcBuffer != nullptr )
			if ( size_t bufferSize = ::GlobalSize( hSrcBuffer ) )
				if ( BYTE* pSrcBuffer = (BYTE*)::GlobalLock( hSrcBuffer ) )
				{
					m_hLockedSrcBuffer = hSrcBuffer;
					Attach( pSrcBuffer, static_cast<UINT>( bufferSize ) );
				}

		if ( nullptr == m_hLockedSrcBuffer )
			AfxThrowOleException( E_POINTER );			// source bufer not accessible
	}

	CGlobalMemFile::CGlobalMemFile( size_t growBytes /*= KiloByte*/ )
		: CMemFile( static_cast<UINT>( growBytes ) )
		, m_hLockedSrcBuffer( nullptr )
	{
	}

	CGlobalMemFile::~CGlobalMemFile()
	{
		if ( m_hLockedSrcBuffer != nullptr )
			::GlobalUnlock( m_hLockedSrcBuffer );
	}

	HGLOBAL CGlobalMemFile::MakeGlobalData( UINT flags /*= GMEM_MOVEABLE*/ )
	{
		HGLOBAL hGlobal = nullptr;
		if ( size_t bufferSize = static_cast<size_t>( GetLength() ) )
		{
			ASSERT_PTR( m_lpBuffer );
			hGlobal = ::GlobalAlloc( flags, bufferSize );
			if ( hGlobal != nullptr )
				if ( void* pDestBuffer = ::GlobalLock( hGlobal ) )
				{
					::CopyMemory( pDestBuffer, m_lpBuffer, bufferSize );
					::GlobalUnlock( hGlobal );
				}
		}
		return hGlobal;
	}
}


namespace serial
{
	fs::CPath GetDocumentPath( const CArchive& archive )
	{
		return path::ExtractPhysical( archive.m_strFileName.GetString() );
	}


	// CScopedLoadingArchive implementation

	int CScopedLoadingArchive::s_latestModelSchema = UnitializedVersion;
	const CArchive* CScopedLoadingArchive::s_pLoadingArchive = nullptr;
	int CScopedLoadingArchive::s_docLoadingModelSchema = UnitializedVersion;

	CScopedLoadingArchive::CScopedLoadingArchive( const CArchive& rArchiveLoading, int docLoadingModelSchema )
	{
		REQUIRE( s_latestModelSchema != UnitializedVersion );		// (!) must have beeen initialized at application startup
		ASSERT_NULL( s_pLoadingArchive );							// nesting of loading archives not allowed
		REQUIRE( rArchiveLoading.IsLoading() );

		s_pLoadingArchive = &rArchiveLoading;
		s_docLoadingModelSchema = docLoadingModelSchema;
	}

	CScopedLoadingArchive::~CScopedLoadingArchive()
	{
		s_pLoadingArchive = nullptr;
		s_docLoadingModelSchema = -1;
	}

	bool CScopedLoadingArchive::IsValidLoadingArchive( const CArchive& rArchive )
	{
		if ( !rArchive.IsLoading() )
			return false;

		if ( IsFileBasedArchive( rArchive ) )
			return s_pLoadingArchive != nullptr;			// must have been created in the scope of loading a FILE with backwards compatibility

		return true;
	}


	// CStreamingGuard implementation

	std::vector<CStreamingGuard*> CStreamingGuard::s_instances;

	CStreamingGuard::CStreamingGuard( const CArchive& rArchive )
		: m_rArchive( rArchive )
		, m_streamingFlags( 0 )
	{
		s_instances.push_back( this );
	}

	CStreamingGuard::~CStreamingGuard()
	{
		ASSERT( s_instances.back() == this );
		s_instances.pop_back();
	}
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()

	CAdapterDocument::CAdapterDocument( serial::IStreamable* pStreamable, const fs::CPath& docPath )
		: CDocument()
		, m_pStreamable( pStreamable )
		, m_pObject( nullptr )
	{
		ASSERT_PTR( m_pStreamable );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	CAdapterDocument::CAdapterDocument( CObject* pObject, const fs::CPath& docPath )
		: CDocument()
		, m_pStreamable( nullptr )
		, m_pObject( pObject )
	{
		ASSERT_PTR( m_pObject );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	void CAdapterDocument::Serialize( CArchive& archive )
	{
		if ( m_pStreamable != nullptr )
		{
			if ( archive.IsLoading() )
				m_pStreamable->Load( archive );
			else
				m_pStreamable->Save( archive );
		}
		else if ( m_pObject != nullptr )
			m_pObject->Serialize( archive );
	}

	bool CAdapterDocument::Load( void ) throws_()
	{
		if ( fs::FileExist( GetPathName() ) )
			if ( OnOpenDocument( GetPathName() ) )
				return true;
			else
				TRACE( _T(" * Error loading document adapter file: %s\n"), GetPathName().GetString() );

		return false;
	}

	bool CAdapterDocument::Save( void ) throws_()
	{
		if ( !OnSaveDocument( GetPathName() ) )
		{
			TRACE( _T(" * Error saving document adapter file: %s\n"), GetPathName().GetString() );
			return false;
		}

		return true;
	}

	void CAdapterDocument::ReportSaveLoadException( const TCHAR* pFilePath, CException* pExc, BOOL isSaving, UINT idDefaultPrompt )
	{
		if ( const CArchiveException* pArchiveExc = dynamic_cast<const CArchiveException*>( pExc ) )
			switch ( pArchiveExc->m_cause )
			{
				case CArchiveException::badSchema:
				case CArchiveException::badClass:
				case CArchiveException::badIndex:
				case CArchiveException::endOfFile:
					ui::MessageBox( str::Format( _T("The loading binary file format is incompatible with current document schema version!\n\n%s"), pFilePath ) );
					return;
			}

		__super::ReportSaveLoadException( pFilePath, pExc, isSaving, idDefaultPrompt );
	}
}
