#ifndef SubjectPredicates_h
#define SubjectPredicates_h
#pragma once

#include "ISubject.h"
#include "ComparePredicates.h"


namespace func
{
	// adapters for comparison predicates

	struct AsCode
	{
		template< typename ObjectType >
		std::tstring operator()( const ObjectType* pObject ) const
		{
			return pObject != NULL ? pObject->GetCode() : std::tstring();
		}
	};
}


namespace pred
{
	typedef CompareScalarAdapterPtr< func::AsCode > CompareCode;
	typedef CompareAdapterPtr< CompareCode, func::DynamicAs< utl::ISubject > > CompareSubjectCode;

	typedef LessPtr< CompareCode > LessCode;


	// for implementing stateful object comparators

	interface IComparator : public IMemoryManaged
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
	IComparator* NewComparator( const Compare& compare )
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
}


#endif // SubjectPredicates_h
