#ifndef FileObjectCache_h
#define FileObjectCache_h
#pragma once

#include <unordered_map>
#include <deque>
#include <afxmt.h>
#include "FileSystem_fwd.h"
#include "MultiThreading.h"
#include "StdHashValue.h"


class CFlagTags;


namespace fs
{
	namespace cache
	{
		enum StatusFlags
		{
			CacheHit		= BIT_FLAG( 0 ),
			RemoveExpired	= BIT_FLAG( 1 ),
			Remove			= BIT_FLAG( 2 ),
			Load			= BIT_FLAG( 3 ),
			LoadingError	= BIT_FLAG( 4 )
		};

		typedef int TStatusFlags;

		const CFlagTags& GetTags_StatusFlags( void );
	}


	// Cache of file-based objects that discards the oldest expired objects.
	// The key is of PathType.
	// Objects are owned by the cache and destroyed with operator delete().
	// Thread safe: access to the cache is serialized internally through a critical section.
	//
	template< typename PathType, typename ObjectType >
	class CFileObjectCache : private utl::noncopyable
	{
	public:
		CFileObjectCache( size_t maxSize ) : m_maxSize( maxSize ) {}
		~CFileObjectCache() { Clear(); }

		void Clear( void );

		size_t GetCount( void ) const;
		const std::deque< PathType >& GetPathKeys( void ) const { return m_expireQueue; }

		bool Contains( const PathType& pathKey, bool checkValid = false ) const { return Find( pathKey, checkValid ) != NULL; }
		ObjectType* Find( const PathType& pathKey, bool checkValid = false ) const;

		bool Add( const PathType& pathKey, ObjectType* pObject )
		{
			mt::CAutoLock lock( &m_cs );
			return _Add( pathKey, pObject );
		}

		bool Remove( const PathType& pathKey )
		{
			mt::CAutoLock lock( &m_cs );
			return _Remove( pathKey, cache::Remove );
		}

		template< typename Iterator >
		void Remove( Iterator itPathKeyStart, Iterator itPathKeyEnd )
		{
			mt::CAutoLock lock( &m_cs );
			for ( ; itPathKeyStart != itPathKeyEnd; ++itPathKeyStart )
				_Remove( *itPathKeyStart );
		}

		typedef std::pair<ObjectType*, CTime> TCachedEntry;		// object, lastModifyTime

		const TCachedEntry* FindEntry( const PathType& pathKey, bool checkValid = false ) const
		{
			mt::CAutoLock lock( &m_cs );
			return _FindEntry( pathKey, checkValid );
		}

		FileExpireStatus CheckExpireStatus( const PathType& pathKey, const TCachedEntry& entry ) const;

		size_t GetMaxSize( void ) const { return m_maxSize; }
		void SetMaxSize( size_t maxSize );
	protected:
		// not synchronized
		const TCachedEntry* _FindEntry( const PathType& pathKey, bool checkValid ) const;
		bool _Add( const PathType& pathKey, ObjectType* pObject );
		bool _Remove( const PathType& pathKey, cache::TStatusFlags cacheFlags = cache::RemoveExpired );
		void _RemoveExpired( void );

		virtual void TraceObject( const PathType& pathKey, ObjectType* pObject, cache::TStatusFlags cacheFlags ) { pathKey, pObject, cacheFlags; }
	private:
		static const CTime& GetLastModifyTime( const TCachedEntry& entry ) { return entry.second; }
		static void DeleteObject( TCachedEntry& entry ) { delete entry.first; }
	private:
		size_t m_maxSize;
		std::unordered_map< PathType, TCachedEntry > m_cachedEntries;
		std::deque< PathType > m_expireQueue;		// at front the oldest, at back the latest
	protected:
		mutable CCriticalSection m_cs;				// serialize cache access for thread safety
	};
}


#endif // FileObjectCache_h
