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

	template< typename IndexT >
	bool CanMoveSelection( size_t itemCount, const std::vector< IndexT >& selIndexes, MoveTo moveTo )
	{
		// assume that selIndexes are pre-sorted
		for ( typename std::vector< IndexT >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
			if ( !CanMoveIndex( itemCount, *itSelIndex, moveTo ) )
				return false;

		return !selIndexes.empty();
	}


	// container-based API (vector, deque, string, etc):

	template< typename ContainerT, typename IndexT >
	inline void MoveBy( ContainerT* pItems, const std::vector< IndexT >& selIndexes, Direction moveBy )
	{
		CSequenceAdapter< typename ContainerT::value_type > sequence( pItems );
		MoveBy( sequence, selIndexes, moveBy );
	}

	template< typename ContainerT, typename IndexT >
	inline void MoveBy( ContainerT* pItems, IndexT selIndex, Direction moveBy )
	{
		CSequenceAdapter< typename ContainerT::value_type > sequence( pItems );
		MoveBy( sequence, selIndex, moveBy );
	}


	template< typename ContainerT, typename IndexT >
	inline void Resequence( ContainerT* pItems, const std::vector< IndexT >& selIndexes, MoveTo moveTo )
	{
		CSequenceAdapter< typename ContainerT::value_type > sequence( pItems );
		Resequence( sequence, selIndexes, moveTo );
	}


	template< typename Type, typename IndexT >
	void MakeDropSequence( std::vector< Type >& rNewSequence, const std::vector< Type >& baselineSeq,
						   IndexT dropIndex, const std::vector< IndexT >& selIndexes )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < baselineSeq.size() );

		rNewSequence = baselineSeq;

		std::vector< Type > revTemp;			// fed in reverse order
		revTemp.reserve( selIndexes.size() );

		// go backwards
		for ( typename std::vector< IndexT >::const_reverse_iterator itIndex = selIndexes.rbegin(); itIndex != selIndexes.rend(); ++itIndex )
		{
			revTemp.push_back( rNewSequence[ *itIndex ] );
			rNewSequence.erase( rNewSequence.begin() + *itIndex );

			if ( *itIndex < dropIndex )
				--dropIndex;
		}

		rNewSequence.insert( rNewSequence.begin() + dropIndex, revTemp.rbegin(), revTemp.rend() );
	}


	template< typename IndexT >
	bool ChangesDropSequenceAt( size_t itemCount, IndexT dropIndex, const std::vector< IndexT >& selIndexes )
	{
		// assume that selIndexes are pre-sorted
		if ( selIndexes.empty() ||
			 selIndexes.size() >= itemCount ||
			 (size_t)dropIndex > itemCount )
			return false;				// invalid selection or drop index

		// generate fake sequence with consecutive indexes - we just need to detect if the sequence changes for drop move
		std::vector< IndexT > baselineSeq( itemCount );				// contains indexes in the range [0, size-1]
		std::generate( baselineSeq.begin(), baselineSeq.end(), GenNumSeq< IndexT >( 0 ) );

		std::vector< IndexT > newSequence;
		MakeDropSequence( newSequence, baselineSeq, dropIndex, selIndexes );
		return newSequence != baselineSeq;
	}


	// CSequenceAdapter-based API:

	// see CSequenceAdapter below for an example of sequence adapter; could be more sophisticated, such as a list ctrl adapter, etc.
	//
	template< typename SequenceT, typename IndexT >
	void MoveBy( SequenceT& rSequence, const std::vector< IndexT >& selIndexes, Direction moveBy )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < rSequence.GetSize() );

		switch ( moveBy )
		{
			case Prev:
				for ( typename std::vector< IndexT >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
					rSequence.Swap( *itSelIndex, *itSelIndex + moveBy );
				break;
			case Next:
				// go backwards when moving down
				for ( typename std::vector< IndexT >::const_reverse_iterator itSelIndex = selIndexes.rbegin(); itSelIndex != selIndexes.rend(); ++itSelIndex )
					rSequence.Swap( *itSelIndex, *itSelIndex + moveBy );
				break;
		}
	}

	template< typename SequenceT, typename IndexT >
	inline void MoveBy( SequenceT& rSequence, IndexT selIndex, Direction moveBy )
	{
		MoveBy( rSequence, std::vector< IndexT >( 1, selIndex ), moveBy );
	}


	template< typename SequenceT, typename IndexT >
	void Resequence( SequenceT& rSequence, const std::vector< IndexT >& selIndexes, MoveTo moveTo )
	{
		// assume that selIndexes are pre-sorted
		ASSERT( !selIndexes.empty() );
		ASSERT( selIndexes.size() < rSequence.GetSize() );

		IndexT lastIndex = static_cast< IndexT >( rSequence.GetSize() - 1 );
		switch ( moveTo )
		{
			case MovePrev:
			case MoveNext:
				MoveBy( rSequence, selIndexes, static_cast< Direction >( moveTo ) );
				break;
			case MoveToStart:
				// shift selected one step back at a time, working on a copy of selected indexes which gets decremented each time
				for ( std::vector< IndexT > indexes = selIndexes; indexes.front() != 0; std::for_each( indexes.begin(), indexes.end(), ModifyBy< Prev >() ) )
					MoveBy( rSequence, indexes, Prev );
				break;
			case MoveToEnd:
				// shift selected one step forth at a time, working on a copy of selected indexes which gets incremented each time
				for ( std::vector< IndexT > indexes = selIndexes; indexes.back() < lastIndex; std::for_each( indexes.begin(), indexes.end(), ModifyBy< Next >() ) )
					MoveBy( rSequence, indexes, Next );
				break;
		}
	}


	// helper classes

	template< Direction direction >
	struct ModifyBy
	{
		template< typename IndexT >
		void operator()( IndexT& rIndex ) { rIndex += static_cast< IndexT >( direction ); }
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
	class CSequenceAdapter
	{
	public:
		typedef Type Type;

		template< typename Container >
		CSequenceAdapter( Container* pSequence ) : m_pSequence( &pSequence->front() ), m_count( pSequence->size() ) { ASSERT_PTR( m_pSequence ); }

		CSequenceAdapter( Type sequence[], size_t count ) : m_pSequence( sequence ), m_count( count ) { ASSERT_PTR( m_pSequence ); }

		void Swap( size_t leftIndex, size_t rightIndex )
		{
			ASSERT( leftIndex < m_count && rightIndex < m_count );
			std::swap( m_pSequence[ leftIndex ], m_pSequence[ rightIndex ] );
		}

		size_t GetSize( void ) const { return m_count; }
	private:
		Type* m_pSequence;
		size_t m_count;
	};
}


#endif // Resequence_hxx
