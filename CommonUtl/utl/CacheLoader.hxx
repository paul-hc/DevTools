#ifndef CacheLoader_hxx
#define CacheLoader_hxx

#include "FileObjectCache.hxx"


namespace fs
{
	// CCacheLoader template code

	using std::placeholders::_1;

	template< typename PathType, typename ObjectType >
	inline CCacheLoader< PathType, ObjectType >::CCacheLoader( size_t maxSize, ICacheOwner< PathType, ObjectType >* pCacheOwner )
		: CFileObjectCache< PathType, ObjectType >( maxSize )
		, m_pCacheOwner( pCacheOwner )
		, m_pendingQueue( std::bind( &CCacheLoader::_Acquire, this, _1 ) )
	{
		ASSERT_PTR( m_pCacheOwner );
	}

	template< typename PathType, typename ObjectType >
	CCacheLoader< PathType, ObjectType >::~CCacheLoader()
	{
	}

	template< typename PathType, typename ObjectType >
	std::pair< ObjectType*, int > CCacheLoader< PathType, ObjectType >::Acquire( const PathType& pathKey )
	{
		mt::CAutoLock lock( &m_cs );

		ObjectType* pObject = NULL;
		int cacheStatus = 0;

		if ( const std::pair< ObjectType*, CTime >* pCachedEntry = FindEntry( pathKey ) )
		{
			fs::FileExpireStatus expireStatus = CheckExpireStatus( pathKey, *pCachedEntry );
			if ( fs::FileNotExpired == expireStatus )
			{
				SetFlag( cacheStatus, cache::CacheHit );
				pObject = pCachedEntry->first;
			}
			else
			{	// fs::ExpiredFileModified, fs::ExpiredFileDeleted
				_Remove( pathKey, 0 );				// delete expired entry, no tracing
				SetFlag( cacheStatus, cache::RemoveExpired );
			}
		}

		if ( NULL == pObject )
		{
			pObject = m_pCacheOwner->LoadObject( pathKey );
			SetFlag( cacheStatus, pObject != NULL ? cache::Load : cache::LoadingError );
		}

		if ( pObject != NULL && !HasFlag( cacheStatus, cache::CacheHit ) )
			_Add( pathKey, pObject );

		std::pair< ObjectType*, int > objectPair( pObject, cacheStatus );

		m_pCacheOwner->TraceObject( pathKey, objectPair.first, objectPair.second );
		return objectPair;
	}

	template< typename PathType, typename ObjectType >
	inline void CCacheLoader< PathType, ObjectType >::_Acquire( const PathType& pathKey )
	{
		Acquire( pathKey );
	}

	template< typename PathType, typename ObjectType >
	cache::EnqueueResult CCacheLoader< PathType, ObjectType >::Enqueue( const PathType& pathKey )
	{
		if ( Contains( pathKey, true ) )
			return cache::Found;

		m_pendingQueue.Enqueue( pathKey );
		return cache::Pending;
	}

	template< typename PathType, typename ObjectType >
	void CCacheLoader< PathType, ObjectType >::Enqueue( const std::vector< PathType >& pathKeys )
	{
		for ( std::vector< PathType >::const_iterator itPathKey = pathKeys.begin(); itPathKey != pathKeys.end(); ++itPathKey )
			Enqueue( *itPathKey );
	}


	// CCacheLoader template code

	template< typename PathType >
	CQueueListener< PathType >::CQueueListener( AcquireFunc acquireFunc )
		: m_acquireFunc( acquireFunc )
		, m_wantExit( false )
		, m_thread( std::bind( &CQueueListener::ListenLoop, this ) )
	{
	}

	template< typename PathType >
	CQueueListener< PathType >::~CQueueListener()
	{
		{
			boost::lock_guard< boost::mutex > lock( m_mutex );
			m_wantExit = true;
			m_queuePending.notify_one();
		}
		m_thread.join();			// wait for the thread to exit
	}

	template< typename PathType >
	void CQueueListener< PathType >::Enqueue( const PathType& pathKey )
	{
		boost::lock_guard< boost::mutex > lock( m_mutex );
		m_queue.push_front( pathKey );
		m_queuePending.notify_one();
	}

	template< typename PathType >
	void CQueueListener< PathType >::WaitCompletePending( void )
	{
		while ( !m_queue.empty() && !m_wantExit )
		{
			boost::lock_guard< boost::mutex > lock( m_mutex );
			m_queuePending.notify_one();
		}
	}

	template< typename PathType >
	void CQueueListener< PathType >::ListenLoop( void )
	{
		mt::CScopedInitializeCom scopedCom;
		PathType pathKey;

		for ( ;; m_queue.pop_back() )
		{
			{
				boost::unique_lock< boost::mutex > lock( m_mutex );
				m_queuePending.wait( lock, std::bind( &CQueueListener::WaitPred, this ) );

				if ( m_wantExit )
					return;

				pathKey = m_queue.back();
			}
			m_acquireFunc( pathKey );
		}
	}

} //namespace fs


#endif // CacheLoader_hxx