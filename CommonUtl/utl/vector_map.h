#ifndef vector_map_h
#define vector_map_h
#pragma once

#include <vector>
#include <functional>


namespace utl
{
	// Adapter for ordered container with an interface similar to std::map using a container with random access such as a std::vector, std::deque, etc.
	// To be used as a replacement for std::map for containers of small number of items, i.e. for which map insertion/deletion/iteration overhead is not justified.
	// It uses binary search assuming the container is ordered by KeyPred.

	template< typename Key, typename Value, typename KeyPred = std::less<Key>, typename Container = std::vector< std::pair<Key, Value> > >
	class vector_map : public Container
	{
	public:
		typedef Container base_type;
		typedef typename base_type::iterator iterator;
		typedef typename base_type::const_iterator const_iterator;

		typedef Key key_type;
		typedef Value mapped_type;
		typedef std::pair<Key, Value> value_type;
		typedef KeyPred key_compare;

		vector_map( void ) {}
		vector_map( const vector_map& right ) : base_type( right ) {}
		explicit vector_map( const Container& container ) : base_type( container ) {}

		template< typename Iterator >
		vector_map( Iterator itFirst, Iterator itLast )
		{
			Assign( itFirst, itLast );
		}

		vector_map& operator=( const vector_map& right )
		{
			base_type::operator=( right );
			return *this;
		}

		const base_type& GetContainer( void ) const { return *this; }

		template< typename Iterator >
		void Assign( Iterator itFirst, Iterator itLast )
		{
			this->clear();

			for ( ; itFirst != itLast; ++itFirst )
				Insert( itFirst->first, itFirst->second );
		}

		bool EraseKey( const Key& key )
		{
			iterator itFound = find( key );
			if ( itFound == this->end() )
				return false;
			this->base_type::erase( itFound );
			return true;
		}

		void PushBack( const Key& key, const Value& value )
		{
			ASSERT( this->upper_bound( key ) == this->end() );		// unique key, must respect the map order
			push_back( value_type( key, value ) );
		}

		iterator Insert( const Key& key, const Value& value )
		{	// replace the value of existing key or insert a new pair
			iterator itWhere = lower_bound( key );
			if ( itWhere == this->end() || m_lessPred( key, itWhere->first ) )
				itWhere = this->insert( itWhere, value_type( key, value ) );
			else
				itWhere->second = value;							// replace value at found the key

			return itWhere;
		}

		bool Contains( const Key& key ) const { return this->find( key ) != this->end(); }

		size_t FindPos( const Key& key ) const
		{	// find an element that matches key
			const_iterator itFound = this->find( key );
			return itFound != this->end() ? std::distance( this->begin(), itFound ) : utl::npos;
		}

		Value& operator[]( const Key& key )
		{	// find element matching key or insert default value
			return Insert( key, Value() )->second;
		}

		iterator find( const Key& key )
		{	// find an element that matches key
			iterator itFound = this->lower_bound( key );
			return itFound == this->end() || m_lessPred( key, itFound->first )
				? this->end() : itFound;
		}

		const_iterator find( const Key& key ) const
		{	// find an element that matches key
			const_iterator itFound = this->lower_bound( key );
			return itFound == this->end() || m_lessPred( key, itFound->first )
				? this->end() : itFound;
		}

		iterator lower_bound( const Key& key )
		{	// find leftmost node not less than key
			return std::lower_bound( this->begin(), this->end(), value_type( key, Value() ), m_lessPred );
		}

		const_iterator lower_bound( const Key& key ) const
		{	// find leftmost node not less than key
			return std::lower_bound( this->begin(), this->end(), value_type( key, Value() ), m_lessPred );
		}

		iterator upper_bound( const Key& key )
		{	// find leftmost node greater than key
			return std::upper_bound( this->begin(), this->end(), value_type( key, Value() ), m_lessPred );
		}

		const_iterator upper_bound( const Key& key ) const
		{	// find leftmost node greater than key
			return std::upper_bound( this->begin(), this->end(), value_type( key, Value() ), m_lessPred );
		}
	private:
		struct SafeKeyPred
		{
			bool operator()( const Key& left, const Key& right ) const
			{
				if ( !m_keyPred( left, right ) )
					return false;
				else
					ASSERT( !m_keyPred( right, left ) );		// key predicate should provide strict ordering
				return true;
			}

			bool operator()( const value_type& left, const value_type& right ) const
			{
				return operator()( left.first, right.first );
			}
		private:
			KeyPred m_keyPred;
		};
	private:
		SafeKeyPred m_lessPred;
	};
}


#endif // vector_map_h
