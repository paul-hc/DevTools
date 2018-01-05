#ifndef FileObjectCache_h
#define FileObjectCache_h
#pragma once

#include <hash_map>
#include <deque>
#include <afxmt.h>


class CFlagTags;


namespace fs
{
	enum FileExpireStatus;


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

		void Add( const PathType& pathKey, ObjectType* pObject ) { mt::CAutoLock lock( &m_cs ); return _Add( pathKey, pObject ); }
		bool Remove( const PathType& pathKey ) { mt::CAutoLock lock( &m_cs ); return _Remove( pathKey, cache::Remove ); }

		template< typename Iterator >
		void Remove( Iterator itPathKeyStart, Iterator itPathKeyEnd )
		{
			mt::CAutoLock lock( &m_cs );
			for ( ; itPathKeyStart != itPathKeyEnd; ++itPathKeyStart )
				_Remove( *itPathKeyStart );
		}

		typedef std::pair< ObjectType*, CTime > CachedEntry;		// object, lastModifyTime

		const CachedEntry* FindEntry( const PathType& pathKey, bool checkValid = false ) const { mt::CAutoLock lock( &m_cs ); return _FindEntry( pathKey, checkValid ); }

		FileExpireStatus CheckExpireStatus( const PathType& pathKey, const CachedEntry& entry ) const;

		size_t GetMaxSize( void ) const { return m_maxSize; }
		void SetMaxSize( size_t maxSize );
	protected:
		// not synchronized
		const CachedEntry* _FindEntry( const PathType& pathKey, bool checkValid ) const;
		void _Add( const PathType& pathKey, ObjectType* pObject );
		bool _Remove( const PathType& pathKey, int cacheFlag = cache::RemoveExpired );
		void _RemoveExpired( void );

		virtual void TraceObject( const PathType& pathKey, ObjectType* pObject, int cacheFlags ) { pathKey, pObject, cacheFlags; }
	private:
		static const CTime& GetLastModifyTime( const CachedEntry& entry ) { return entry.second; }
		static void DeleteObject( CachedEntry& entry ) { delete entry.first; }
	private:
		size_t m_maxSize;
		stdext::hash_map< PathType, CachedEntry > m_cachedEntries;
		std::deque< PathType > m_expireQueue;		// at front the oldest, at back the latest
	protected:
		mutable CCriticalSection m_cs;				// serialize cache access for thread safety
	};
}


#endif // FileObjectCache_h
