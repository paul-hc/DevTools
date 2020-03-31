#ifndef FileObjectCache_hxx
#define FileObjectCache_hxx

#include "ContainerUtilities.h"
#include "MfcUtilities.h"
#include "StructuredStorage.h"


namespace fs
{
	template< typename PathType, typename ObjectType >
	inline void CFileObjectCache< PathType, ObjectType >::Clear( void )
	{
		mt::CAutoLock lock( &m_cs );
		for ( typename stdext::hash_map< PathType, CachedEntry >::iterator it = m_cachedEntries.begin(); it != m_cachedEntries.end(); ++it )
			DeleteObject( it->second );
		m_cachedEntries.clear();
		m_expireQueue.clear();
	}

	template< typename PathType, typename ObjectType >
	inline size_t CFileObjectCache< PathType, ObjectType >::GetCount( void ) const
	{
		mt::CAutoLock lock( &m_cs );
		ASSERT( m_cachedEntries.size() == m_expireQueue.size() );
		return m_cachedEntries.size();
	}

	template< typename PathType, typename ObjectType >
	inline ObjectType* CFileObjectCache< PathType, ObjectType >::Find( const PathType& pathKey, bool checkValid /*= false*/ ) const
	{
		mt::CAutoLock lock( &m_cs );		// nested lock works fine (no deadlock) within the same calling thread

		const CachedEntry* pCachedEntry = _FindEntry( pathKey, checkValid );
		return pCachedEntry != NULL ? pCachedEntry->first : NULL;
	}

	template< typename PathType, typename ObjectType >
	typename const CFileObjectCache< PathType, ObjectType >::CachedEntry*
	CFileObjectCache< PathType, ObjectType >::_FindEntry( const PathType& pathKey, bool checkValid ) const
	{
		stdext::hash_map< PathType, CachedEntry >::const_iterator itFound = m_cachedEntries.find( pathKey );
		if ( itFound != m_cachedEntries.end() )
			if ( !checkValid || fs::FileNotExpired == CheckExpireStatus( pathKey, itFound->second ) )
				return &itFound->second;

		return NULL;
	}

	template< typename PathType, typename ObjectType >
	bool CFileObjectCache< PathType, ObjectType >::_Add( const PathType& pathKey, ObjectType* pObject )
	{
		mt::CAutoLock lock( &m_cs );

		ASSERT_PTR( pObject );
		ASSERT( m_cachedEntries.size() == m_expireQueue.size() );				// consistent

		// bug fix [2020-03-31] - sometimes _Add collides with an existing thumb
		//ASSERT( m_cachedEntries.find( pathKey ) == m_cachedEntries.end() );		// must be new entry (before the fix above)
		stdext::hash_map< PathType, CachedEntry >::const_iterator itFound = m_cachedEntries.find( pathKey );
		if ( itFound != m_cachedEntries.end() )
			if ( itFound->second.first == pObject )
			{
				TRACE( _T("[?] Attempt to add an already cached thumbnail for: ") ); TraceObject( pathKey, itFound->second.first, cache::CacheHit );
				return false;			// skip caching same thumb if already cached
			}
			else
				_Remove( pathKey );

		ASSERT( !utl::Contains( m_expireQueue, pathKey ) );

		m_cachedEntries[ pathKey ] = std::make_pair( pObject, fs::ReadLastModifyTime( fs::CastFlexPath( pathKey ) ) );
		m_expireQueue.push_back( pathKey );

		_RemoveExpired();
		return true;					// thumb cached
	}

	template< typename PathType, typename ObjectType >
	bool CFileObjectCache< PathType, ObjectType >::_Remove( const PathType& pathKey, int cacheFlag /*= cache::RemoveExpired*/ )
	{
		REQUIRE( m_cachedEntries.size() == m_expireQueue.size() );			// consistent

		stdext::hash_map< PathType, CachedEntry >::iterator itFound = m_cachedEntries.find( pathKey );
		if ( itFound == m_cachedEntries.end() )
			return false;

		TraceObject( pathKey, itFound->second.first, cacheFlag );

		DeleteObject( itFound->second );
		m_cachedEntries.erase( itFound );
		utl::RemoveExisting( m_expireQueue, pathKey );
		return true;
	}

	template< typename PathType, typename ObjectType >
	void CFileObjectCache< PathType, ObjectType >::_RemoveExpired( void )
	{
		size_t removeChunkSize = std::max( m_maxSize / 10, (size_t)2 );

		// remove oldest expired (from the front of m_expireQueue)
		if ( m_cachedEntries.size() > m_maxSize )
			for ( size_t i = removeChunkSize; i-- != 0; )
				VERIFY( _Remove( m_expireQueue.front() ) );

		ENSURE( m_cachedEntries.size() == m_expireQueue.size() );			// consistent
	}

	template< typename PathType, typename ObjectType >
	inline FileExpireStatus CFileObjectCache< PathType, ObjectType >::CheckExpireStatus( const PathType& pathKey, const CachedEntry& entry ) const
	{
		mt::CAutoLock lock( &m_cs );

		return fs::CheckExpireStatus( fs::CastFlexPath( pathKey ), GetLastModifyTime( entry ) );
	}

	template< typename PathType, typename ObjectType >
	inline void CFileObjectCache< PathType, ObjectType >::SetMaxSize( size_t maxSize )
	{
		mt::CAutoLock lock( &m_cs );
		m_maxSize = maxSize;
		_RemoveExpired();
	}


} //namespace fs


#endif // FileObjectCache_hxx
