#ifndef MfcUtilities_h
#define MfcUtilities_h
#pragma once

#include "utl/MultiThreading.h"
#include "utl/Timer.h"


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


namespace fs { class CPath; }


namespace serial
{
	interface IStreamable;


	inline bool IsFileBasedArchive( const CArchive& rArchive ) { return !rArchive.m_strFileName.IsEmpty(); }
	fs::CPath GetDocumentPath( const CArchive& archive );


	// Must have been created in the scope of loading a FILE with backwards compatibility.
	// Not necessary to create when loading from a CMemFile archive - it will assume s_latestModelSchema.
	//
	class CScopedLoadingArchive
	{
	public:
		CScopedLoadingArchive( const CArchive& rLoadingArchive, int fileLoadingModelSchema );
		~CScopedLoadingArchive();

		static void SetLatestModelSchema( int latestModelSchema ) { s_latestModelSchema = latestModelSchema; }

		static bool IsValidLoadingArchive( const CArchive& rArchive );

		template< typename EnumType >
		static EnumType GetModelSchema( const CArchive& rArchive )
		{
			ASSERT( s_latestModelSchema != UnitializedVersion );		// (!) must have beeen initialized at application startup
			ASSERT( IsValidLoadingArchive( rArchive ) );				// if a file archive, ensure CScopedLoadingArchive is created in the scope of loading

			if ( &rArchive == s_pLoadingArchive )
				return static_cast< EnumType >( s_fileLoadingModelSchema );

			return static_cast< EnumType >( s_latestModelSchema );
		}
	private:
		enum { UnitializedVersion = -1 };

		static int s_latestModelSchema;

		// file loading only
		static const CArchive* s_pLoadingArchive;
		static int s_fileLoadingModelSchema;
	};


	class CStreamingTimeGuard : private utl::noncopyable
	{
	public:
		CStreamingTimeGuard( const CArchive& archive );
		~CStreamingTimeGuard();

		double GetElapsedSeconds( void ) const { return m_timer.ElapsedSeconds(); }
		bool IsTimeout( double timeout ) const { return GetElapsedSeconds() > timeout; }
		const CArchive& GetArchive( void ) const { return m_rArchive; }

		CTimer& GetTimer( void ) { return m_timer; }

		bool HasStreamingFlag( int flag ) const { return HasFlag( m_streamingFlags, flag ); }
		void SetStreamingFlag( int flag, bool on = true ) { SetFlag( m_streamingFlags, flag, on ); }

		static CStreamingTimeGuard* GetTop( void ) { return !s_instances.empty() ? s_instances.back() : NULL; }
	private:
		const CArchive& m_rArchive;
		CTimer m_timer;
		int m_streamingFlags;		// client code maintains the actual flags

		static std::vector< CStreamingTimeGuard* > s_instances;		// could be stacked, the deepest at the top (back)
	};
}


namespace ui
{
	// takes advantage of safe saving through a CMirrorFile provided by CDocument; redirects to m_pObject->Serialize()
	//
	class CAdapterDocument : public CDocument
	{
	public:
		CAdapterDocument( serial::IStreamable* pStreamable, const fs::CPath& docPath );
		CAdapterDocument( CObject* pObject, const fs::CPath& docPath );
		virtual ~CAdapterDocument() {}

		bool Load( void ) throws_();
		bool Save( void ) throws_();

		virtual void ReportSaveLoadException( const TCHAR* pFilePath, CException* pExc, BOOL isSaving, UINT idDefaultPrompt );
	protected:
		// base overrides
		virtual void Serialize( CArchive& archive );
	private:
		serial::IStreamable* m_pStreamable;			// use either one or the other
		CObject* m_pObject;
	};
}


namespace ui
{
	template< typename ViewT >
	ViewT* FindDocumentView( const CDocument* pDoc )
	{
		ASSERT_PTR( pDoc );

		for ( POSITION pos = pDoc->GetFirstViewPosition(); pos != NULL; )
			if ( ViewT* pView = dynamic_cast< ViewT* >( pDoc->GetNextView( pos ) ) )
				return pView;

		return NULL;
	}
}


#endif // MfcUtilities_h
