#ifndef FileSystem_fwd_h
#define FileSystem_fwd_h
#pragma once

#include "FlagSet.h"
#include "Path_fwd.h"


namespace utl { interface ICounter; }


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath );
	bool IsValidDirectory( const TCHAR* pDirPath );
	bool IsValidShellLink( const TCHAR* pFilePath );
}


namespace fs
{
	struct CFileState;


	enum EnumFlags
	{
		EF_Recurse				= BIT_FLAG( 0 ),
		EF_IgnoreFiles			= BIT_FLAG( 1 ),
		EF_IgnoreHiddenNodes	= BIT_FLAG( 2 ),
		EF_NoSortSubDirs		= BIT_FLAG( 3 ),

		EF_ResolveShellLinks	= BIT_FLAG( 8 )
	};

	typedef utl::CFlagSet<EnumFlags> TEnumFlags;


	interface IEnumerator
	{
		virtual const TEnumFlags& GetEnumFlags( void ) const = 0;

		virtual void AddFoundFile( const fs::CPath& filePath ) = 0;
		virtual bool AddFoundSubDir( const fs::TDirPath& subDirPath ) = 0;

		// advanced overrideables
		virtual bool CanIncludeNode( const fs::CFileState& nodeState ) const = 0;
		virtual bool CanRecurse( void ) const = 0;
		virtual bool MustStop( void ) const = 0;					// abort searching?
		virtual utl::ICounter* GetDepthCounter( void ) = 0;			// supports recursion depth


		// default implementation

		bool HasEnumFlag( EnumFlags enumFlag ) const { return GetEnumFlags().Has( enumFlag ); }

		virtual void OnAddFileInfo( const fs::CFileState& fileState );	// override to get access to extra file state
	};


	abstract class IEnumeratorImpl : public IEnumerator, private utl::noncopyable		// default implementation for advanced overrideables
	{
	protected:
		IEnumeratorImpl( fs::TEnumFlags enumFlags = fs::TEnumFlags() ) : m_enumFlags( enumFlags ) {}
	public:
		// IEnumerator interface (partial)
		virtual const TEnumFlags& GetEnumFlags( void ) const { return m_enumFlags; }
		virtual bool CanIncludeNode( const fs::CFileState& nodeState ) const;
		virtual bool CanRecurse( void ) const { return m_enumFlags.Has( fs::EF_Recurse ); }
		virtual bool MustStop( void ) const { return false; }
		virtual utl::ICounter* GetDepthCounter( void ) { return nullptr; }
	private:
		fs::TEnumFlags m_enumFlags;
	};


	template< typename PathContainerT >
	void SortPathsDirsFirst( PathContainerT& rPaths, bool ascending = true );
}


#include "Path.h"


class CEnumTags;


namespace fs
{
	enum FileExpireStatus { FileNotExpired, ExpiredFileModified, ExpiredFileDeleted };

	const CEnumTags& GetTags_FileExpireStatus( void );


	enum TimeField { CreatedDate, ModifiedDate, AccessedDate, _TimeFieldCount };

	const CEnumTags& GetTags_TimeField( void );

	enum AcquireResult { FoundExisting, Created, CreationError };		// result of resource acquisition

	enum FileContentMatch
	{
		FileSize,				// quick and approximate
		FileSizeAndCrc32		// slower but accurate
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
