
#include "stdafx.h"
#include "ResourceData.h"
#include "Image_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CResourceData::CResourceData( const TCHAR* pResId, const TCHAR* pResType )
	: m_hInst( CScopedResInst::Find( pResId, pResType ) )				// determine location of the resource
	, m_hResource( m_hInst != NULL ? ::FindResource( m_hInst, pResId, pResType ) : NULL )
	, m_hGlobal( m_hResource != NULL ? ::LoadResource( m_hInst, m_hResource ) : NULL )
	, m_pResource( m_hGlobal != NULL ? ::LockResource( m_hGlobal ) : NULL )
{
}

CResourceData::~CResourceData()
{
	if ( IsValid() )
	{
		ASSERT_PTR( m_hGlobal );
		::UnlockResource( m_hGlobal );
		::FreeResource( m_hGlobal );
	}
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
