#ifndef CacheLoader_h
#define CacheLoader_h
#pragma once

#include "FileObjectCache.h"
#include "StdThread.h"


namespace fs
{
	namespace cache
	{
		enum EnqueueResult { Found, Pending };
	}


	template< typename PathType > class CQueueListener;


	template< typename PathType, typename ObjectType >
	interface ICacheOwner
	{
		virtual ObjectType* LoadObject( const PathType& pathKey ) = 0;
		virtual void TraceObject( const PathType& pathKey, ObjectType* pObject, cache::TStatusFlags cacheFlags ) = 0;
	};


	template< typename PathType, typename ObjectType >
	class CCacheLoader : public CFileObjectCache<PathType, ObjectType>
	{
		typedef CFileObjectCache<PathType, ObjectType> TFileObjectCache;
	public:
		using TFileObjectCache::TCachedEntry;

		typedef std::function< ObjectType*( const PathType& ) > TLoadFunc;
		typedef std::function< void( const std::pair<ObjectType*, cache::TStatusFlags>&, const PathType& ) > TTraceFunc;

		CCacheLoader( size_t maxSize, ICacheOwner<PathType, ObjectType>* pCacheOwner );
		~CCacheLoader();

		std::pair<ObjectType*, cache::TStatusFlags> Acquire( const PathType& pathKey );		// object, cacheStatusFlags

		cache::EnqueueResult Enqueue( const PathType& pathKey );
		void Enqueue( const std::vector<PathType>& pathKeys );
		void WaitPendingQueue( void ) { m_pendingQueue.WaitCompletePending(); }
	protected:
		// base overrides
		virtual void TraceObject( const PathType& pathKey, ObjectType* pObject, cache::TStatusFlags cacheFlags )
		{
			m_pCacheOwner->TraceObject( pathKey, pObject, cacheFlags );
		}
	private:
		void _Acquire( const PathType& pathKey );
	private:
		ICacheOwner<PathType, ObjectType>* m_pCacheOwner;
		CQueueListener<PathType> m_pendingQueue;
	};


	template< typename PathType >
	class CQueueListener : private utl::noncopyable
	{
	public:
		typedef std::function< void( const PathType& ) > TAcquireFunc;

		CQueueListener( TAcquireFunc acquireFunc );
		~CQueueListener();

		void Enqueue( const PathType& pathKey );
		void WaitCompletePending( void );
	private:
		void ListenLoop( void );

		bool WaitPred( void ) const { return m_wantExit || !m_queue.empty(); }
		bool WaitProcessPred( void ) const { return m_wantExit || m_queue.empty(); }
	private:
		TAcquireFunc m_acquireFunc;
		bool m_wantExit;
		std::thread m_thread;
		std::mutex m_mutex;
		std::condition_variable m_queuePending;
		std::deque<PathType> m_queue;
	};
}


#endif // CacheLoader_h
