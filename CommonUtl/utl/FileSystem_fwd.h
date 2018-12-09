#ifndef FileSystem_fwd_h
#define FileSystem_fwd_h
#pragma once

#include "Path.h"


class CEnumTags;


namespace fs
{
	enum TimeField { CreatedDate, ModifiedDate, AccessedDate, _TimeFieldCount };

	const CEnumTags& GetTags_TimeField( void );

	enum AcquireResult { FoundExisting, Created, CreationError };		// result of resource acquisition


	bool IsValidFile( const TCHAR* pFilePath );
	bool IsValidDirectory( const TCHAR* pDirPath );


	interface IEnumerator
	{
		virtual void AddFoundFile( const TCHAR* pFilePath ) = 0;
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) { pSubDirPath; }

		// advanced, provides extra info
		virtual void AddFile( const CFileFind& foundFile ) { AddFoundFile( foundFile.GetFilePath() ); }

		// override to find first file, then abort searching
		virtual bool MustStop( void ) { return false; }
	};


	class CHandle : private utl::noncopyable
	{
	public:
		CHandle( HANDLE handle = INVALID_HANDLE_VALUE ) : m_handle( handle ) {}
		~CHandle() { Close(); }

		bool IsValid( void ) const { return m_handle != INVALID_HANDLE_VALUE; }
		HANDLE Get( void ) const { ASSERT( IsValid() ); return m_handle; }
		HANDLE* GetPtr( void ) { return &m_handle; }

		bool Close( void ) { return !IsValid() || ::CloseHandle( Release() ) != FALSE; }
		void Reset( HANDLE handle = INVALID_HANDLE_VALUE ) { Close(); m_handle = handle; }

		HANDLE Release( void )
		{
			HANDLE handle = m_handle;
			m_handle = INVALID_HANDLE_VALUE;
			return handle;
		}
	private:
		HANDLE m_handle;
	};
}


#endif // FileSystem_fwd_h
