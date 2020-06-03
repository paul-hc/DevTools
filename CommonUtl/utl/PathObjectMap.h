#ifndef PathObjectMap_h
#define PathObjectMap_h
#pragma once

#include <hash_map>
#include "StdHashValue.h"


namespace fs
{
	// Simple map of PathT keys to file-based objects by value. No ownership, not thread-safe.
	//
	template< typename PathT, typename ValueT >
	class CPathMap : private utl::noncopyable
	{
	public:
		CPathMap( void ) {}
		~CPathMap() { Clear(); }

		void Clear( void ) { m_pathMap.clear(); }

		size_t GetCount( void ) const { return m_pathMap.size(); }
		bool Contains( const PathT& pathKey ) const { return Find( pathKey ) != NULL; }

		const ValueT* Find( const PathT& pathKey ) const
		{
			stdext::hash_map< PathT, ValueT >::const_iterator itFound = m_pathMap.find( pathKey );
			return itFound != m_pathMap.end() ? &itFound->second : NULL;
		}

		ValueT* Find( const PathT& pathKey )
		{
			stdext::hash_map< PathT, ValueT >::iterator itFound = m_pathMap.find( pathKey );
			return itFound != m_pathMap.end() ? &itFound->second : NULL;
		}

		ValueT& Lookup( const PathT& pathKey )
		{
			ValueT* pFound = Find( pathKey );
			ASSERT_PTR( pFound );
			return *pFound;
		}

		void Add( const PathT& pathKey, const ValueT& value )
		{
			REQUIRE( !Contains( pathKey ) );
			m_pathMap[ pathKey ] = value;
		}

		bool Remove( const PathT& pathKey )
		{
			stdext::hash_map< PathT, ValueT >::iterator itFound = m_pathMap.find( pathKey );
			if ( itFound == m_pathMap.end() )
				return false;

			m_pathMap.erase( itFound );
			return true;
		}

		template< typename Iterator >
		void Remove( Iterator itPathKeyStart, Iterator itPathKeyEnd )
		{
			for ( ; itPathKeyStart != itPathKeyEnd; ++itPathKeyStart )
				Remove( *itPathKeyStart );
		}
	protected:
		stdext::hash_map< PathT, ValueT > m_pathMap;
	};
}


namespace func
{
	struct NoDeleteEntry
	{
		template< typename EntryPairT >
		void operator()( const EntryPairT& rEntryPair ) const
		{
			rEntryPair;
		}
	};


	struct DeleteEntryObject
	{
		template< typename EntryPairT >
		void operator()( const EntryPairT& rEntryPair ) const
		{
			delete rEntryPair.first;		// delete the object
		}
	};


	struct DeleteEntry
	{
		template< typename EntryPairT >
		void operator()( const EntryPairT& rEntryPair ) const
		{
			delete rEntryPair.first;		// delete the object
			delete rEntryPair.second;		// delete the param
		}
	};
}


#include "MultiThreading.h"


namespace fs
{
	// Map of file-based objects that stores entries of object pointers with an optional data parameter associated.
	// The key is of PathT.
	// Entry ownership is optional, controlled by DeleteEntryFunc.
	// Thread safe: access to the cache is serialized internally through a critical section.
	//
	template< typename PathT, typename ObjectT, typename ParamT = void*, typename DeleteEntryFunc = func::NoDeleteEntry >
	class CPathObjectMap : private utl::noncopyable
	{
	public:
		typedef std::pair< ObjectT*, ParamT > TEntry;		// object, param

		CPathObjectMap( void ) {}
		~CPathObjectMap() { Clear(); }

		void Clear( void )
		{
			mt::CAutoLock lock( &m_cs );
			std::for_each( m_pathMap.begin(), m_pathMap.end(), m_deleteFunc );
			m_pathMap.clear();
		}

		size_t GetCount( void ) const
		{
			mt::CAutoLock lock( &m_cs );
			return m_pathMap.size();
		}

		bool Contains( const PathT& pathKey ) const { return FindEntry( pathKey ) != NULL; }

		ObjectT* Find( const PathT& pathKey ) const
		{
			const TEntry* pFoundEntry = FindEntry( pathKey );
			return pFoundEntry != NULL ? pFoundEntry->first : NULL;
		}

		const TEntry* FindEntry( const PathT& pathKey ) const
		{
			mt::CAutoLock lock( &m_cs );		// nested locks works fine (no deadlock) within the same calling thread
			return _FindEntry( pathKey );
		}

		void Add( const PathT& pathKey, ObjectT* pObject, const ParamT& param = ParamT() )
		{
			mt::CAutoLock lock( &m_cs );
			ASSERT_PTR( pObject );
			_AddEntry( pathKey, TEntry( pObject, param ) );
		}

		bool Remove( const PathT& pathKey )
		{
			mt::CAutoLock lock( &m_cs );
			return _RemoveEntry( pathKey );
		}

		template< typename Iterator >
		void Remove( Iterator itPathKeyStart, Iterator itPathKeyEnd )
		{
			mt::CAutoLock lock( &m_cs );
			for ( ; itPathKeyStart != itPathKeyEnd; ++itPathKeyStart )
				_RemoveEntry( *itPathKeyStart );
		}

		const ParamT& LookupParam( const PathT& pathKey ) const
		{
			const TEntry* pFoundEntry = FindEntry( pathKey );
			ASSERT_PTR( pFoundEntry );
			return pFoundEntry->second;
		}

		bool StoreParam( const PathT& pathKey, const ParamT& param )
		{
			mt::CAutoLock lock( &m_cs );		// nested locks works fine (no deadlock) within the same calling thread
			TEntry* pFoundEntry = const_cast< TEntry* >( _FindEntry( pathKey ) );
			if ( NULL == pFoundEntry )
				return false;

			pFoundEntry->second = param;
			return true;
		}
	protected:
		// un-synchronized methods
		const TEntry* _FindEntry( const PathT& pathKey ) const
		{
			stdext::hash_map< PathT, TEntry >::const_iterator itFound = m_pathMap.find( pathKey );
			if ( itFound != m_pathMap.end() )
				return &itFound->second;

			return NULL;
		}

		void _AddEntry( const PathT& pathKey, const TEntry& entry )
		{
			ASSERT( !Contains( pathKey ) );
			m_pathMap[ pathKey ] = entry;
		}

		bool _RemoveEntry( const PathT& pathKey )
		{
			stdext::hash_map< PathT, TEntry >::iterator itFound = m_pathMap.find( pathKey );
			if ( itFound == m_pathMap.end() )
				return false;

			DeleteEntry( itFound->second );
			m_pathMap.erase( itFound );
			return true;
		}
	private:
		void DeleteEntry( TEntry& entry ) { m_deleteFunc( entry ); }
	private:
		stdext::hash_map< PathT, TEntry > m_pathMap;
		DeleteEntryFunc m_deleteFunc;
	protected:
		mutable CCriticalSection m_cs;				// serialize cache access for thread safety
	};
}


#endif // PathObjectMap_h
