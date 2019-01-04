
#ifndef StdStl_h
#define StdStl_h
#pragma once


#define OEMRESOURCE			// load OCR_NORMAL cursor values from winuser.h
#include <afx.h>			// must include first to prevent: uafxcw.lib(afxmem.obj) : error LNK2005: "void * __cdecl operator new[](unsigned int)" (??_U@YAPAXI@Z) already defined in libcpmt.lib(newaop.obj)


// C standard headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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
#include <typeinfo.h>
#include <utility>
#include <vector>
#include <functional>
#include <memory>
#include <iterator>
#include <algorithm>
#include <string>
#include <sstream>
#include <iosfwd>


#pragma warning( pop )				// restore to the initial warning level


namespace std
{
	using namespace tr1;


#ifdef _UNICODE
	typedef wstring tstring;
	typedef std::wios tios;
	typedef wstringbuf tstringbuf;
	typedef wistream tistream;
	typedef wistringstream tistringstream;
	typedef wifstream tifstream;
	typedef wostream tostream;
	typedef wostringstream tostringstream;
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
	typedef ofstream tofstream;

	#define tcout cout
	#define tcerr cerr
	#define tcin cin
#endif
}


// include this in stdafx.h just before first Windows header: #include <afxwin.h>

namespace utl
{
	template< typename LeftT, typename RightT >
	LeftT max( const LeftT& left, const RightT& right ) { return ( left < static_cast< LeftT >( right ) ) ? static_cast< LeftT >( right ) : left; }

	template< typename LeftT, typename RightT >
	LeftT min( const LeftT& left, const RightT& right ) { return ( static_cast< LeftT >( right ) < left ) ? static_cast< LeftT >( right ) : left; }
}


#ifdef NOMINMAX
	// required by some Windows headers: return by value to make it compatible with uses such as: max< int, unsigned long >( a, b )
	using utl::max;
	using utl::min;
#endif //NOMINMAX


#endif // StdStl_h
