#ifndef Functional_h
#define Functional_h
#pragma once

#include <functional>


namespace func
{
	// unary functor adapter for setter (non-const) methods with 1 argument - for no arguments use: std::mem_fun( &ObjectT::SetMethod )

	template< typename ResultT, typename ObjectT, typename Arg1T >
	class Setter1 : public std::binary_function<ObjectT*, Arg1T, ResultT>
	{
	public:
		explicit Setter1( ResultT (ObjectT::*pSetMethod)( Arg1T ), Arg1T value ) : m_pSetMethod( pSetMethod ), m_value( value ) {}

		ResultT operator()( ObjectT* pObject ) const	// for containers of pointers
		{
			return (pObject->*m_pSetMethod)( m_value );
		}

		ResultT operator()( ObjectT& rObject ) const	// for containers of objects by value
		{
			return (rObject.*m_pSetMethod)( m_value );
		}
	private:
		ResultT (ObjectT::*m_pSetMethod)( Arg1T );		// the method pointer
		Arg1T m_value;
	};


	// unary functor adapter for non-const methods with 1 argument - for no arguments use: std::mem_fun( &ObjectT::SetMethod )

	template< typename ResultT, typename ObjectT, typename Arg1T, typename Arg2T >
	class Setter2 : public std::binary_function<ObjectT*, Arg1T, ResultT>
	{
	public:
		explicit Setter2( ResultT (ObjectT::*pSetMethod)( Arg1T, Arg2T ), Arg1T arg1, Arg2T arg2 ) : m_pSetMethod( pSetMethod ), m_arg1( arg1 ), m_arg2( arg2 ) {}

		ResultT operator()( ObjectT* pObject ) const	// for containers of pointers
		{
			return (pObject->*m_pSetMethod)( m_arg1, m_arg2 );
		}

		ResultT operator()( ObjectT& rObject ) const	// for containers of objects by value
		{
			return (rObject.*m_pSetMethod)( m_arg1, m_arg2 );
		}
	private:
		ResultT (ObjectT::*m_pSetMethod)( Arg1T, Arg2T );		// the method pointer
		Arg1T m_arg1;
		Arg2T m_arg2;
	};


	template< typename Arg1T, typename ResultT, typename ObjectT >
	Setter1<ResultT, ObjectT, Arg1T> MakeSetter( ResultT (ObjectT::*pSetMethod)( Arg1T ), Arg1T arg1 )
	{
		return Setter1<ResultT, ObjectT, Arg1T>( pSetMethod, arg1 );		// bound to 1 argument value
	}

	template< typename Arg1T, typename Arg2T, typename ResultT, typename ObjectT >
	Setter2<ResultT, ObjectT, Arg1T, Arg2T> MakeSetter( ResultT (ObjectT::*pSetMethod)( Arg1T, Arg2T ), Arg1T arg1, Arg2T arg2 )
	{	// Arg1T and Arg2T template arguments come first, so this could be explicitly specialized for e.g. 'const std::tstring&' arguments
		return Setter2<ResultT, ObjectT, Arg1T, Arg2T>( pSetMethod, arg1, arg2 );	// bound to 2 argument values
	}
}


#endif // Functional_h
