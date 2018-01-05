#ifndef Resequence_hxx
#define Resequence_hxx

#include "ContainerUtilities.h"
#include "Resequence.h"


namespace seq
{
	inline bool CanMoveIndex( size_t itemCount, size_t index, MoveTo moveTo )
	{
		ASSERT( index < itemCount );

		switch ( moveTo )
		{
			case MoveToStart: return index != 0;
			case MoveToEnd:	  return index != ( itemCount - 1 );
		}

		size_t newIndex = index + moveTo;
		return newIndex < itemCount && newIndex != index;		// in bounds and changed?
	}

	inline bool CanMoveIndexBy( size_t itemCount, size_t index, Direction direction )
	{
		return CanMoveIndex( itemCount, index, static_cast< MoveTo >( direction ) );
	}

	template< typename IndexType >
	bool CanMoveSelection( size_t itemCount, const std::vector< IndexType >& selIndexes, MoveTo moveTo )
	{
		// assume that selIndexes are pre-sorted
		for ( typename std::vector< IndexType >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
			if ( !CanMoveIndex( itemCount, *itSelIndex, moveTo ) )
				return false;

		return !selIndexes.empty();
	}


	// see CArraySequence below for an example of sequence adapter; could be more sophisticated, such as a list ctrl adapter, etc.
	//
	template< typename Sequence, typename IndexType >
	void MoveBy( Sequence& rSequence, const std::vector< IndexType >& selIndexes, Direction direction )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < rSequence.GetSize() );

		switch ( direction )
		{
			case Prev:
				for ( typename std::vector< IndexType >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
					rSequence.Swap( *itSelIndex, *itSelIndex + direction );
				break;
			case Next:
				// go backwards when moving down
				for ( typename std::vector< IndexType >::const_reverse_iterator itSelIndex = selIndexes.rbegin(); itSelIndex != selIndexes.rend(); ++itSelIndex )
					rSequence.Swap( *itSelIndex, *itSelIndex + direction );
				break;
		}
	}


	template< typename Sequence, typename IndexType >
	void Resequence( Sequence& rSequence, const std::vector< IndexType >& selIndexes, MoveTo moveTo )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < rSequence.GetSize() );

		IndexType lastIndex = static_cast< IndexType >( rSequence.GetSize() - 1 );
		switch ( moveTo )
		{
			case MovePrev:
			case MoveNext:
				MoveBy( rSequence, selIndexes, static_cast< Direction >( moveTo ) );
				break;
			case MoveToStart:
				// shift selected one step back at a time, working on a copy of selected indexes which gets decremented each time
				for ( std::vector< IndexType > indexes = selIndexes; indexes.front() != 0; std::for_each( indexes.begin(), indexes.end(), ModifyBy< Prev >() ) )
					MoveBy( rSequence, indexes, Prev );
				break;
			case MoveToEnd:
				// shift selected one step forth at a time, working on a copy of selected indexes which gets incremented each time
				for ( std::vector< IndexType > indexes = selIndexes; indexes.back() < lastIndex; std::for_each( indexes.begin(), indexes.end(), ModifyBy< Next >() ) )
					MoveBy( rSequence, indexes, Next );
				break;
		}
	}


	template< typename Type, typename IndexType >
	void MakeDropSequence( std::vector< Type >& rNewSequence, const std::vector< Type >& baselineSeq,
						   IndexType dropIndex, const std::vector< IndexType >& selIndexes )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < baselineSeq.size() );

		rNewSequence = baselineSeq;

		std::vector< Type > revTemp;			// fed in reverse order
		revTemp.reserve( selIndexes.size() );

		// go backwards
		for ( typename std::vector< IndexType >::const_reverse_iterator itIndex = selIndexes.rbegin(); itIndex != selIndexes.rend(); ++itIndex )
		{
			revTemp.push_back( rNewSequence[ *itIndex ] );
			rNewSequence.erase( rNewSequence.begin() + *itIndex );
			if ( *itIndex < dropIndex )
				--dropIndex;
		}

		rNewSequence.insert( rNewSequence.begin() + dropIndex, revTemp.rbegin(), revTemp.rend() );
	}


	template< typename IndexType >
	bool ChangesDropSequenceAt( size_t itemCount, IndexType dropIndex, const std::vector< IndexType >& selIndexes )
	{
		// assume that selIndexes are pre-sorted
		if ( selIndexes.empty() ||
			 selIndexes.size() >= itemCount ||
			 (size_t)dropIndex > itemCount )
			return false;				// invalid selection or drop index

		// generate fake sequence with consecutive indexes - we just need to detect if the sequence changes for drop move
		std::vector< IndexType > baselineSeq( itemCount );				// contains indexes in the range [0, size-1]
		std::generate( baselineSeq.begin(), baselineSeq.end(), GenNumSeq< IndexType >( 0 ) );

		std::vector< IndexType > newSequence;
		MakeDropSequence( newSequence, baselineSeq, dropIndex, selIndexes );
		return newSequence != baselineSeq;
	}


	// helper classes

	template< Direction direction >
	struct ModifyBy
	{
		template< typename IndexType >
		void operator()( IndexType& rIndex ) { rIndex += static_cast< IndexType >( direction ); }
	};


	template< typename Type >
	struct GenNumSeq
	{
		GenNumSeq( Type initialValue = Type() ) : m_value( initialValue ) {}

		Type operator()( void ) { return m_value++; }
	private:
		int m_value;
	};


	// adapter for swapping items in a array-like sequence (array, vector, deque, string, etc)
	//
	template< typename Type >
	class CArraySequence
	{
	public:
		typedef Type Type;

		template< typename Container >
		CArraySequence( Container* pSequence ) : m_pSequence( &pSequence->front() ), m_size( pSequence->size() ) { ASSERT_PTR( m_pSequence ); }

		CArraySequence( Type sequence[], size_t size ) : m_pSequence( sequence ), m_size( size ) { ASSERT_PTR( m_pSequence ); }

		void Swap( size_t leftIndex, size_t rightIndex )
		{
			ASSERT( leftIndex < m_size && rightIndex < m_size );
			std::swap( m_pSequence[ leftIndex ], m_pSequence[ rightIndex ] );
		}

		size_t GetSize( void ) const { return m_size; }
	private:
		Type* m_pSequence;
		size_t m_size;
	};

}


#endif // Resequence_hxx
