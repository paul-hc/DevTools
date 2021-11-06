#ifndef Resequence_hxx
#define Resequence_hxx

#include "ContainerUtilities.h"
#include "Resequence.h"
#include <list>


namespace seq
{
	inline bool CanMoveIndex( size_t itemCount, size_t index, MoveTo moveTo )
	{
		REQUIRE( index < itemCount );

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
		return CanMoveIndex( itemCount, index, static_cast<MoveTo>( direction ) );
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
}


namespace seq
{
	// Drag & Drop resequence

	template< typename IndexT >
	bool ChangesDropSequenceAt( size_t itemCount, IndexT dropIndex, const std::vector< IndexT >& dragSelIndexes )
	{
		REQUIRE( utl::IsOrdered( dragSelIndexes ) );	// must be pre-sorted

		if ( dragSelIndexes.empty() ||
			 dragSelIndexes.size() >= itemCount ||
			 (size_t)dropIndex > itemCount )
			return false;				// invalid selection or drop index

		// generate fake sequence with consecutive indexes - we just need to detect if the sequence changes for drop move
		std::vector< IndexT > baselineSeq( itemCount );				// contains indexes in the range [0, size-1]
		std::generate( baselineSeq.begin(), baselineSeq.end(), func::GenNumSeq< IndexT >( 0 ) );

		std::vector< IndexT > newSequence;
		MakeDropSequence( newSequence, baselineSeq, dropIndex, dragSelIndexes );
		return newSequence != baselineSeq;
	}


	// Drop semantics: returns the new (adjusted) dropIndex
	//
	template< typename Type, typename IndexT >
	IndexT MakeDropSequence( std::vector< Type >& rNewSequence, const std::vector< Type >& baselineSeq,
							 IndexT dropIndex, const std::vector< IndexT >& dragSelIndexes,
							 std::vector< IndexT >* pDroppedSelIndexes = NULL )					// optional output
	{
		REQUIRE( utl::IsOrdered( dragSelIndexes ) );	// must be pre-sorted
		REQUIRE( !dragSelIndexes.empty() );
		REQUIRE( dragSelIndexes.size() < baselineSeq.size() );

		size_t dropPos = static_cast<size_t>( dropIndex );
		REQUIRE( dropPos <= baselineSeq.size() );		// if dropPos == count then drop-append objects

		rNewSequence = baselineSeq;

		std::list< Type > selTemp;						// selected objects (in order)

		// iterate in reverse selected order
		for ( typename std::vector< IndexT >::const_reverse_iterator itSelIndex = dragSelIndexes.rbegin(); itSelIndex != dragSelIndexes.rend(); ++itSelIndex )
		{
			size_t selPos = static_cast<size_t>( *itSelIndex );
			REQUIRE( selPos < baselineSeq.size() );				// valid selected index?

			selTemp.push_front( rNewSequence[ selPos ] );		// feed at front since we reverse-iterate
			rNewSequence.erase( rNewSequence.begin() + selPos );

			if ( selPos < dropPos )		// selPos before dropped position?
				--dropPos;				// adjust down dropped-pos
		}
		ENSURE( selTemp.size() == dragSelIndexes.size() );

		rNewSequence.insert( rNewSequence.begin() + dropPos, selTemp.begin(), selTemp.end() );		// 'drop' the selected object

		if ( pDroppedSelIndexes != NULL )
		{	// adjust dropped selected indexes according to the new dropped-index
			pDroppedSelIndexes->resize( dragSelIndexes.size() );
			std::generate( pDroppedSelIndexes->begin(), pDroppedSelIndexes->end(), func::GenNumSeq< IndexT >( static_cast<IndexT>( dropPos ) ) );
		}

		ENSURE( utl::SameContents( rNewSequence, baselineSeq ) );

		return static_cast<IndexT>( dropPos );			// new (adjusted) dropIndex
	}


	// Undo Drop semantics: uses the ORIGINAL drag selection and dropped index (after drop); returns the original (rolled-back) dropIndex.
	//
	template< typename Type, typename IndexT >
	IndexT MakeUndoDropSequence( std::vector< Type >& rNewSequence, const std::vector< Type >& baselineSeq,
								 IndexT droppedIndex, const std::vector< IndexT >& origDragSelIndexes )
	{
		REQUIRE( utl::IsOrdered( origDragSelIndexes ) );		// must be pre-sorted
		REQUIRE( !origDragSelIndexes.empty() );
		REQUIRE( origDragSelIndexes.size() < baselineSeq.size() );

		size_t dropPos = static_cast<size_t>( droppedIndex );
		REQUIRE( dropPos < baselineSeq.size() );

		rNewSequence = baselineSeq;

		typename std::vector< Type >::iterator itSelStart = rNewSequence.begin() + dropPos;
		typename std::vector< Type >::iterator itSelEnd = itSelStart + origDragSelIndexes.size();

		// cut the dropped selection: contiguous starting at dropPos
		std::vector< Type > selTemp( itSelStart, itSelEnd );		// selected objects (in order)
		rNewSequence.erase( itSelStart, itSelEnd );

		for ( size_t i = 0; i != selTemp.size(); ++i )
		{
			size_t selPos = static_cast<size_t>( origDragSelIndexes[ i ] );

			rNewSequence.insert( rNewSequence.begin() + selPos, selTemp[ i ] );

			if ( selPos < dropPos )		// originally shifted down?
				++dropPos;				// un-adjust up (roll-back)
		}

		ENSURE( utl::SameContents( rNewSequence, baselineSeq ) );

		return static_cast<IndexT>( dropPos );			// old (rolled-back) dropIndex
	}
}


namespace seq
{
	// CSequenceAdapter-based API:

	// see CSequenceAdapter below for an example of sequence adapter; could be more sophisticated, such as a list ctrl adapter, etc.
	//
	template< typename SequenceT, typename IndexT >
	void MoveBy( SequenceT& rSequence, const std::vector< IndexT >& selIndexes, Direction moveBy )
	{
		REQUIRE( utl::IsOrdered( selIndexes ) );		// must be pre-sorted
		REQUIRE( !selIndexes.empty() );
		REQUIRE( selIndexes.size() < rSequence.GetSize() );

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
		REQUIRE( utl::IsOrdered( selIndexes ) );		// must be pre-sorted
		REQUIRE( !selIndexes.empty() );
		REQUIRE( selIndexes.size() < rSequence.GetSize() );

		IndexT lastIndex = static_cast<IndexT>( rSequence.GetSize() - 1 );
		switch ( moveTo )
		{
			case MovePrev:
			case MoveNext:
				MoveBy( rSequence, selIndexes, static_cast<Direction>( moveTo ) );
				break;
			case MoveToStart:
				// shift selected one step back at a time, working on a copy of selected indexes which gets decremented each time
				for ( std::vector< IndexT > indexes = selIndexes;
					  indexes.front() != 0;
					  std::for_each( indexes.begin(), indexes.end(), ModifyBy< Prev >() ) )
					MoveBy( rSequence, indexes, Prev );
				break;
			case MoveToEnd:
				// shift selected one step forth at a time, working on a copy of selected indexes which gets incremented each time
				for ( std::vector< IndexT > indexes = selIndexes;
					  indexes.back() < lastIndex;
					  std::for_each( indexes.begin(), indexes.end(), ModifyBy< Next >() ) )
					MoveBy( rSequence, indexes, Next );
				break;
		}
	}
}


namespace seq
{
	// helper classes

	template< Direction direction >
	struct ModifyBy
	{
		template< typename IndexT >
		void operator()( IndexT& rIndex ) { rIndex += static_cast<IndexT>( direction ); }
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
			REQUIRE( leftIndex < m_count && rightIndex < m_count );
			std::swap( m_pSequence[ leftIndex ], m_pSequence[ rightIndex ] );
		}

		size_t GetSize( void ) const { return m_count; }
	private:
		Type* m_pSequence;
		size_t m_count;
	};
}


#endif // Resequence_hxx
