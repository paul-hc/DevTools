
#include "stdafx.h"
#include "MfcUtilities.h"
#include "Path.h"
#include "Serialization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	CGlobalMemFile::CGlobalMemFile( HGLOBAL hSrcBuffer ) throws_( CException )
		: CMemFile( 0 )				// no growth when reading
		, m_hLockedSrcBuffer( NULL )
	{
		if ( hSrcBuffer != NULL )
			if ( size_t bufferSize = ::GlobalSize( hSrcBuffer ) )
				if ( BYTE* pSrcBuffer = (BYTE*)::GlobalLock( hSrcBuffer ) )
				{
					m_hLockedSrcBuffer = hSrcBuffer;
					Attach( pSrcBuffer, static_cast< UINT >( bufferSize ) );
				}

		if ( NULL == m_hLockedSrcBuffer )
			AfxThrowOleException( E_POINTER );			// source bufer not accessible
	}

	CGlobalMemFile::CGlobalMemFile( size_t growBytes /*= KiloByte*/ )
		: CMemFile( static_cast< UINT >( growBytes ) )
		, m_hLockedSrcBuffer( NULL )
	{
	}

	CGlobalMemFile::~CGlobalMemFile()
	{
		if ( m_hLockedSrcBuffer != NULL )
			::GlobalUnlock( m_hLockedSrcBuffer );
	}

	HGLOBAL CGlobalMemFile::MakeGlobalData( UINT flags /*= GMEM_MOVEABLE*/ )
	{
		HGLOBAL hGlobal = NULL;
		if ( size_t bufferSize = static_cast< size_t >( GetLength() ) )
		{
			ASSERT_PTR( m_lpBuffer );
			hGlobal = ::GlobalAlloc( flags, bufferSize );
			if ( hGlobal != NULL )
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
	// CScopedLoadingArchive implementation

	std::pair< const CArchive*, int > CScopedLoadingArchive::s_loadingArchive( NULL, 0 );
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()

	CAdapterDocument::CAdapterDocument( serial::IStreamable* pStreamable, const fs::CPath& docPath )
		: m_pStreamable( pStreamable )
		, m_pObject( NULL )
	{
		ASSERT_PTR( m_pStreamable );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	CAdapterDocument::CAdapterDocument( CObject* pObject, const fs::CPath& docPath )
		: m_pStreamable( NULL )
		, m_pObject( pObject )
	{
		ASSERT_PTR( m_pObject );
		SetPathName( docPath.GetPtr(), FALSE );		// no MRU for this
		m_bAutoDelete = false;						// use this document as auto variable
	}

	void CAdapterDocument::Serialize( CArchive& archive )
	{
		if ( m_pStreamable != NULL )
		{
			if ( archive.IsLoading() )
				m_pStreamable->Load( archive );
			else
				m_pStreamable->Save( archive );
		}
		else if ( m_pObject != NULL )
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
		if ( OnSaveDocument( GetPathName() ) )
			return true;

		TRACE( _T(" * Error saving document adapter file: %s\n"), GetPathName().GetString() );
		return false;
	}
}