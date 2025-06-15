
#include "pch.h"
#include "ResourceData.h"
#include "Image_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CResourceData::CResourceData( const TCHAR* pResName /*= nullptr*/, const TCHAR* pResType /*= nullptr*/ )
	: m_hInst( nullptr )				// determine location of the resource
	, m_hResource( nullptr )
	, m_hGlobal( nullptr )
	, m_pResource( nullptr )
{
	if ( pResName != nullptr )
		LoadResource( pResName, pResType );
}

CResourceData::~CResourceData()
{
	Clear();
}

void CResourceData::Clear( void )
{
	if ( IsValid() )
	{
		ASSERT_PTR( m_hGlobal );
		::UnlockResource( m_hGlobal );
		::FreeResource( m_hGlobal );

		m_hInst = nullptr;
		m_hResource = nullptr;
		m_hGlobal = nullptr;
		m_pResource = nullptr;
	}
}

bool CResourceData::LoadResource( const TCHAR* pResName, const TCHAR* pResType )
{
	Clear();

	if ( ( m_hInst = CScopedResInst::Find( pResName, pResType ) ) != nullptr )				// determine location of the resource
		if ( ( m_hResource = ::FindResource( m_hInst, pResName, pResType ) ) != nullptr )
			if ( ( m_hGlobal = ::LoadResource( m_hInst, m_hResource ) ) != nullptr )
				if ( ( m_pResource = ::LockResource( m_hGlobal ) ) != nullptr )
					return true;

	return false;
}

CComPtr<IStream> CResourceData::CreateStreamCopy( void ) const
{
	CComPtr<IStream> pStream;
	DWORD resSize = GetSize();

	if ( resSize != 0 )
		if ( HGLOBAL hResCopy = ::GlobalAlloc( GMEM_MOVEABLE, resSize ) )					// CreateStreamOnHGlobal requires an in-memory copy of the resource
		{
			void* pResCopy = ::GlobalLock( hResCopy );
			ASSERT_PTR( pResCopy );

			::CopyMemory( pResCopy, m_pResource, resSize );

			if ( !HR_OK( ::CreateStreamOnHGlobal( hResCopy, TRUE, &pStream ) ) )			// delete on Release()
				delete pResCopy;
		}

	return pStream;
}
