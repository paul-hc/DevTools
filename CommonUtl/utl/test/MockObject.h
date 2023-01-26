#ifndef MockObject_h
#define MockObject_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


namespace str
{
	// FWD:

	template< typename CharT, typename ValueT >
	std::basic_string<CharT> FormatValue( const ValueT& value );
}


namespace ut
{
	abstract class CMockObject		// polymorphic base
	{
	protected:
		CMockObject( void ) { REQUIRE( s_instanceCount >= 0 ); ++s_instanceCount; }
	public:
		virtual ~CMockObject() { --s_instanceCount; ENSURE( s_instanceCount >= 0 ); }

		virtual std::string Format( void ) const = 0;

		// object counts
		static int GetInstanceCount( void ) { REQUIRE( s_instanceCount >= 0 ); return s_instanceCount; }
		static bool HasInstances( void ) { return GetInstanceCount() != 0; }
	private:
		static int s_instanceCount;			// keep track of total instance count (for deleting algorithms)
	};


	template< typename ValueT >
	class CMockValue : public CMockObject
	{
	public:
		CMockValue( ValueT value = ValueT() ) : m_value( value ) {}
		virtual ~CMockValue() {}

		bool operator<( const CMockValue& right ) const { return m_value < right.m_value; }

		const ValueT& GetValue( void ) const { return m_value; }
		void SetValue( ValueT value ) { m_value = value; }

		// base overrides
		virtual std::string Format( void ) const { return str::FormatValue<char>( m_value ); }
	private:
		ValueT m_value;
	};


	typedef CMockValue<int> TMockInt;
}


inline std::ostream& operator<<( std::ostream& os, const ut::CMockObject& item ) { return os << item.Format(); }

template< typename ValueT >
inline std::ostream& operator<<( std::ostream& os, const ut::CMockValue<ValueT>& item ) { return os << item.GetValue(); }


#include "ComparePredicates.h"


namespace pred
{
	struct CompareMockItem
	{
		template< typename ValueT >
		CompareResult operator()( const ut::CMockValue<ValueT>& left, const ut::CMockValue<ValueT>& right ) const
		{
			return Compare_Scalar( left, right );
		}
	};

	typedef CompareAdapter< CompareMockItem, func::PtrToReference > TCompareMockItemPtr;
}


namespace func
{
	struct MockToString
	{
		template< typename ValueT >
		std::string operator()( const ut::CMockValue<ValueT>& item ) const { return item.Format(); }

		template< typename ValueT >
		std::string operator()( const ut::CMockValue<ValueT>* pItem ) const { return pItem->Format(); }
	};


	template< typename NumericT >
	struct GenNewMockSeq : public std::unary_function< void, NumericT >
	{
		GenNewMockSeq( NumericT initialValue = NumericT(), NumericT step = 1 ) : m_value( initialValue ), m_step( step ) {}

		ut::CMockValue<NumericT>* operator()( void )
		{
			ut::CMockValue<NumericT>* pNewMockValue = new ut::CMockValue<NumericT>( m_value );
			m_value += m_step;
			return pNewMockValue;
		}
	private:
		NumericT m_value;
		NumericT m_step;
	};
}


#endif //USE_UT


#endif // MockObject_h
