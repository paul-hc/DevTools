#ifndef vector_map_h
#define vector_map_h
#pragma once

#include <vector>
#include <functional>


namespace utl
{
	// Adapter for ordered containers with an interface similar to std::map using a container with random access such as a std::vector, std::deque, etc.
	// To be used as a replacement for std::map for containers of small number of items, i.e. for which map insertion/deletion/iteration overhead is not justified.
	// It uses binary search assuming the container is ordered by KeyPred.

	template< typename Key, typename Value, typename KeyPred = std::less< Key >, typename Container = std::vector< std::pair< Key, Value > > >
	class vector_map : public Container
	{
	public:
		typedef Container base_type;
		typedef typename base_type::iterator iterator;
		typedef typename base_type::const_iterator const_iterator;

		typedef Key key_type;
		typedef std::pair< Key, Value > value_type;
		typedef KeyPred key_compare;

		vector_map( void ) {}
		vector_map( const vector_map& right ) : base_type( right ) {}
		explicit vector_map( const Container& container ) : base_type( container ) {}

		template< typename Iterator >
		vector_map( Iterator itFirst, Iterator itLast ) : base_type( itFirst, itLast ) {}

		vector_map& operator=( const vector_map& right )
		{
			base_type::operator=( right );
			return *this;
		}

		Value& operator[]( const Key& key )
		{	// find element matching key or insert default value
			iterator itWhere = lower_bound( key );
			if ( itWhere == this->end() || !KeyEquals( key, itWhere->first ) )
				itWhere = this->insert( itWhere, value_type( key, Value() ) );

			return itWhere->second;
		}

		iterator find( const Key& key )
		{	// find an element that matches key
			iterator itFound = lower_bound( key );
			return itFound == this->end() || !KeyEquals( key, itFound->first )
				? this->end() : itFound;
		}

		const_iterator find( const Key& key ) const
		{	// find an element that matches key
			const_iterator itFound = lower_bound( key );
			return itFound == this->end() || !KeyEquals( key, itFound->first )
				? this->end() : itFound;
		}

		iterator lower_bound( const Key& key )
		{	// find leftmost node not less than key
			return std::lower_bound( this->begin(), this->end(), value_type( key, Value() ), m_keyPairPred );
		}

		const_iterator lower_bound( const Key& key ) const
		{	// find leftmost node not less than key
			return std::lower_bound( this->begin(), this->end(), value_type( key, Value() ), m_keyPairPred );
		}

		iterator upper_bound(const Key& key)
		{	// find leftmost node greater than key
			return std::upper_bound( this->begin(), this->end(), value_type( key, Value() ), m_keyPairPred );
		}

		const_iterator upper_bound(const Key& key) const
		{	// find leftmost node greater than key
			return std::upper_bound( this->begin(), this->end(), value_type( key, Value() ), m_keyPairPred );
		}

		bool EraseKey( const Key& key )
		{
			iterator itFound = find( key );
			if ( itFound == this->end() )
				return false;
			this->base_type::erase( itFound );
			return true;
		}
	private:
		bool KeyEquals( const Key& left, const Key& right ) const
		{
			return !m_keyPred( left, right ) && !m_keyPred( right, left );
		}

		bool KeyEquals( const value_type& left, const value_type& right ) const
		{
			return KeyEquals( left.first, right.first );
		}

		struct KeyPairPred : public std::binary_function< value_type, value_type, bool >
		{
			bool operator()( const value_type& left, const value_type& right ) const
			{
				return m_keyPred( left.first, right.first );
			}
		private:
			KeyPred m_keyPred;
		};
	private:
		KeyPred m_keyPred;
		KeyPairPred m_keyPairPred;
	};
}


#endif // vector_map_h