#ifndef StdStl_h
#define StdStl_h
#pragma once


#if _MSC_VER >= VS_2015		// MSVC++ 14.0+ (Visual Studio 2015)
	#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING		// suppress warning - error C4996: 'std::tr1': warning STL4002: The non-Standard std::tr1 namespace and TR1-only machinery are deprecated and will be REMOVED.
#endif


#define OEMRESOURCE			// load OCR_NORMAL cursor values from winuser.h
#include <afx.h>			// must include first to prevent: uafxcw.lib(afxmem.obj) : error LNK2005: "void * __cdecl operator new[](unsigned int)" (??_U@YAPAXI@Z) already defined in libcpmt.lib(newaop.obj)


// C standard headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>


#pragma warning( push, 3 )			// switch to warning level 3

#pragma message( "STL << Microsoft" )


// STL main warning disables
#pragma warning( disable: 4786 )	// identifier was truncated to 'number' characters in the debug information
#pragma warning( disable: 4231 )	// nonstandard extension used : 'identifier' before template explicit instantiation

// Warning Level 4 STL disables:

#pragma warning( disable: 4018 )	// signed/unsigned mismatch
#pragma warning( disable: 4100 )	// unreferenced formal parameter
#pragma warning( disable: 4146 )	// unary minus operator applied to unsigned type, result still unsigned
#pragma warning( disable: 4244 )	// conversion from 'unsigned int' to 'char', possible loss of data
#pragma warning( disable: 4511 )	// copy constructor could not be generated
#pragma warning( disable: 4512 )	// assignment operator could not be generated
#pragma warning( disable: 4663 )	// C++ language change: to explicitly specialize class template use the following syntax


#include <yvals.h>
#include <typeinfo>
#include <utility>
#include <vector>
#include <functional>
#include <memory>
#include <iterator>
#include <algorithm>
#include <string>
#include <sstream>
#include <iosfwd>

#include "CppVer.h"			// for IS_CPP_11

#if !defined(IS_CPP_11) && defined(USE_BOOST)
	#include <boost/move/unique_ptr.hpp>	// practical replacement for C++ 03
#endif

#pragma warning( pop )				// restore to the initial warning level


namespace std
{
#ifndef IS_CPP_11
	using namespace tr1;

	#ifdef USE_BOOST
		using boost::movelib::unique_ptr;
	#endif

	template< typename Type, size_t size >
	inline const Type* begin( Type (&_array)[size] )
	{
		return _array;
	}

	template< typename Type, size_t size >
	inline const Type* end( Type (&_array)[size] )
	{
		return _array + size;
	}
#endif // !IS_CPP_11


#ifdef _UNICODE
	typedef wstring tstring;
	typedef std::wios tios;
	typedef wstringbuf tstringbuf;
	typedef wistream tistream;
	typedef wistringstream tistringstream;
	typedef wifstream tifstream;
	typedef wostream tostream;
	typedef wostringstream tostringstream;
	typedef wstringstream tstringstream;
	typedef wofstream tofstream;

	#define tcout wcout
	#define tcerr wcerr
	#define tcin wcin
#else
	typedef string tstring;
	typedef std::ios tios;
	typedef stringbuf tstringbuf;
	typedef istream tistream;
	typedef istringstream tistringstream;
	typedef ifstream tifstream;
	typedef ostream tostream;
	typedef ostringstream tostringstream;
	typedef stringstream tstringstream;
	typedef ofstream tofstream;

	#define tcout cout
	#define tcerr cerr
	#define tcin cin
#endif


#if _MSC_VER <= VS_2008		// MSVC++ 9.0 (Visual Studio 2008)

template< typename BidIterT >
inline reverse_iterator<BidIterT> make_reverse_iterator( BidIterT it )
{	// missing in VC9
	return reverse_iterator<BidIterT>( it );
}

#endif // VS_2008
}


// include this in pch.h just before first Windows header: #include <afxwin.h>

namespace utl
{
	__declspec(selectany) extern const size_t npos = std::tstring::npos;


	// general algorithms

	template< typename ContainerT, typename FuncT >
	inline FuncT for_each( IN OUT ContainerT& rObjects, FuncT func )
	{
		return std::for_each( rObjects.begin(), rObjects.end(), func );
	}

	template< typename ContainerT, typename FuncT >
	inline FuncT for_each( const ContainerT& objects, FuncT func )
	{
		return std::for_each( objects.begin(), objects.end(), func );
	}


	// copy items between containers using a conversion functor (unary)

	template< typename DestContainerT, typename SrcContainerT, typename CvtFuncT >
	inline void transform( const SrcContainerT& srcItems, OUT DestContainerT& rDestItems, CvtFuncT cvtFunc )
	{
		std::transform( srcItems.begin(), srcItems.end(), std::inserter( rDestItems, rDestItems.end() ), cvtFunc );
	}

	template< typename ContainerT, typename UnaryFuncT >
	inline void generate( OUT ContainerT& rItems, UnaryFuncT genFunc )
	{
		std::generate( rItems.begin(), rItems.end(), genFunc );		// replace [first, last) with genFunc
	}
}


namespace utl
{
	template< typename LeftT, typename RightT >
	LeftT max( const LeftT& left, const RightT& right ) { return ( left < static_cast<LeftT>( right ) ) ? static_cast<LeftT>( right ) : left; }

	template< typename LeftT, typename RightT >
	LeftT min( const LeftT& left, const RightT& right ) { return ( static_cast<LeftT>( right ) < left ) ? static_cast<LeftT>( right ) : left; }


	template< typename DiffT, typename IteratorT >
	inline DiffT Distance( IteratorT itFirst, IteratorT itLast )
	{
		return static_cast<DiffT>( std::distance( itFirst, itLast ) );
	}


	template< typename ValueT >
	inline std::pair<ValueT, ValueT> make_pair_single( const ValueT& value )		// return pair composed from single value
	{
		return std::pair<ValueT, ValueT>( value, value );
	}


	// container bounds: works with std::list (not random iterator)

	template< typename ContainerT >
	inline const typename ContainerT::value_type& Front( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Front( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline const typename ContainerT::value_type& Back( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Back( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }
}


#ifdef NOMINMAX
	// required by some Windows headers: return by value to make it compatible with uses such as: max<int, unsigned long>( a, b )
	using utl::max;
	using utl::min;
#endif //NOMINMAX


#endif // StdStl_h
