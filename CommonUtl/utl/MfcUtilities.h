#ifndef MfcUtilities_h
#define MfcUtilities_h
#pragma once

#include <afxmt.h>


namespace com
{
	// wrapper for a PROPVARIANT value
	//
	class CPropVariant
	{
	public:
		CPropVariant( void ) { ::PropVariantInit( &m_value ); }
		~CPropVariant() { Clear(); }

		const PROPVARIANT& Get( void ) const { return m_value; }
		void Clear( void ) { ::PropVariantClear( &m_value ); }

		PROPVARIANT* operator&( void ) { Clear(); return &m_value; }		// clear before passing variant to get new value

		template< typename ValueType >
		bool GetByteAs( ValueType* pValue )
		{
			if ( m_value.vt != VT_UI1 )
				return false;
			*pValue = static_cast< ValueType >( m_value.bVal );
			return true;
		}

		template< typename ValueType >
		bool GetWordAs( ValueType* pValue )
		{
			if ( m_value.vt != VT_UI2 )
				return false;
			*pValue = static_cast< ValueType >( m_value.uiVal );
			return true;
		}

		bool GetBool( bool* pValue )
		{
			if ( m_value.vt != VT_BOOL )
				return false;
			*pValue = m_value.boolVal != FALSE;
			return true;
		}
	private:
		PROPVARIANT m_value;
	};
}


namespace mt
{
	// same as ::CSingleLock, but lock automatically on constructor

	class CAutoLock : public CSingleLock
	{
	public:
		explicit CAutoLock( CSyncObject* pObject ) : CSingleLock( pObject, TRUE ) {}
	};


	// initialize COM in the current thread (worker or UI thread)

	class CScopedInitializeCom
	{
	public:
		CScopedInitializeCom( void );
		~CScopedInitializeCom() { Uninitialize(); }

		void Uninitialize( void );
	private:
		bool m_comInitialized;
	};
}


namespace utl
{
	template< typename Type >
	HGLOBAL CopyToGlobalData( const Type array[], size_t count, UINT flags = GMEM_MOVEABLE )
	{
		if ( HGLOBAL hGlobal = ::GlobalAlloc( flags, count * sizeof( Type ) ) )
		{
			if ( void* pDestData = ::GlobalLock( hGlobal ) )
			{
				::CopyMemory( pDestData, array, count * sizeof( Type ) );
				::GlobalUnlock( hGlobal );
				return hGlobal;
			}
			::GlobalFree( hGlobal );		// free the memory on error
		}
		return NULL;
	}


	// operates on global memory; used for clipboard and drag-drop transfers
	//
	class CGlobalMemFile : public CMemFile
	{
	public:
		CGlobalMemFile( HGLOBAL hSrcBuffer ) throws_( CException );		// reading
		CGlobalMemFile( size_t growBytes = KiloByte );					// writing
		virtual ~CGlobalMemFile();

		HGLOBAL MakeGlobalData( UINT flags = GMEM_MOVEABLE );			// end of writing: allocate and copy buffer contents
	private:
		HGLOBAL m_hLockedSrcBuffer;										// locked buffer, used only when reading
	};
}


namespace serial
{
	interface IStreamable;


	class CScopedLoadingArchive
	{
	public:
		CScopedLoadingArchive( const CArchive* pArchive, int version )
		{
			ASSERT_NULL( s_loadingArchive.first );			// nesting of loading archives not allowed
			ASSERT_PTR( pArchive );
			ASSERT( pArchive->IsLoading() );
			s_loadingArchive.first = pArchive;
			s_loadingArchive.second = version;
		}

		~CScopedLoadingArchive()
		{
			s_loadingArchive.first = NULL;
			s_loadingArchive.second = 0;
		}

		template< typename EnumType >
		static EnumType GetVersion( const CArchive* pArchive, EnumType defaultVersion )
		{
			ASSERT_PTR( pArchive );
			return s_loadingArchive.first == pArchive ? static_cast< EnumType >( s_loadingArchive.second ) : defaultVersion;
		}
	private:
		static std::pair< const CArchive*, int > s_loadingArchive;
	};
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()

	class CAdapterDocument : public CDocument
	{
	public:
		CAdapterDocument( serial::IStreamable* pStreamable, const std::tstring& docPath );
		CAdapterDocument( CObject* pObject, const std::tstring& docPath );
		virtual ~CAdapterDocument() {}

		bool Load( void ) throws_();
		bool Save( void ) throws_();
	protected:
		// base overrides
		virtual void Serialize( CArchive& archive );
	private:
		serial::IStreamable* m_pStreamable;			// use either one or the other
		CObject* m_pObject;
	};
}


#endif // MfcUtilities_h
