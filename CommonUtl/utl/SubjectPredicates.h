#ifndef SubjectPredicates_h
#define SubjectPredicates_h
#pragma once

#include "ISubject.h"
#include "ComparePredicates.h"


namespace pred
{
	typedef CompareScalarAdapterPtr< func::AsCode > CompareCode;
	typedef CompareScalarAdapterPtr< func::AsDisplayCode > CompareDisplayCode;
	typedef CompareAdapterPtr< CompareCode, func::DynamicAs< utl::ISubject > > CompareSubjectCode;

	typedef LessPtr< CompareCode > LessCode;


	// for implementing stateful object comparators

	interface IComparator : public utl::IMemoryManaged
	{
		virtual CompareResult CompareObjects( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const = 0;
	};


	// implements pred::IComparator in terms of Compare

	template< typename Compare >
	struct Comparator : public IComparator
	{
		Comparator( Compare compare = Compare() ) : m_compare( compare ) {}
		virtual ~Comparator() {}

		// IComparator interface
		virtual CompareResult CompareObjects( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const
		{
			return m_compare( pLeft, pRight );
		}
	private:
		Compare m_compare;
	};


	template< typename Compare >
	inline IComparator* NewComparator( const Compare& compare )
	{
		return new Comparator< Compare >( compare );
	}


	// use a pred::IComparator interface in sorting predicates

	struct ComparatorProxy
	{
		ComparatorProxy( const IComparator* pComparator ) : m_pComparator( pComparator ) { ASSERT_PTR( m_pComparator ); }

		CompareResult operator()( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const
		{
			return m_pComparator->CompareObjects( pLeft, pRight );
		}
	private:
		const IComparator* m_pComparator;
	};


	// implements pred::IComparator in terms of GetPropFunc

	template< typename ObjectType, typename GetPropFunc, typename Compare = pred::CompareValue >
	struct PropertyComparator : public IComparator
	{
		PropertyComparator( GetPropFunc getPropFunc, Compare compare = Compare() )
			: m_getPropFunc( getPropFunc )
			, m_compare( compare )
		{
		}

		// IComparator interface
		virtual CompareResult CompareObjects( const utl::ISubject* pLeft, const utl::ISubject* pRight ) const
		{
			return m_compare( m_getPropFunc( AsObject( pLeft ) ), m_getPropFunc( AsObject( pRight ) ) );
		}
	private:
		static const ObjectType* AsObject( const utl::ISubject* pSubject ) { return checked_static_cast< const ObjectType* >( pSubject ); }
	private:
		GetPropFunc m_getPropFunc;
		Compare m_compare;
	};


	template< typename ObjectType, typename GetPropFunc >
	IComparator* NewPropertyComparator( GetPropFunc getPropFunc )
	{
		return new PropertyComparator< ObjectType, GetPropFunc >( getPropFunc );		// use default pred::CompareValue
	}

	template< typename ObjectType, typename Compare, typename GetPropFunc >
	IComparator* NewPropertyComparator( GetPropFunc getPropFunc )
	{
		return new PropertyComparator< ObjectType, GetPropFunc, Compare >( getPropFunc );
	}
}


#endif // SubjectPredicates_h
