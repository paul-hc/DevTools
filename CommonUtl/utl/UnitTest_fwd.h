#ifndef UnitTest_fwd_h
#define UnitTest_fwd_h
#pragma once

#include <iosfwd>


// FWD stream inserters (for C++ 11 and up):
namespace ut
{
	class CMockObject;
	template< typename ValueT > class CMockValue;
}

std::ostream& operator<<( std::ostream& os, const ut::CMockObject& item );

template< typename ValueT >
std::ostream& operator<<( std::ostream& os, const ut::CMockValue<ValueT>& item );


#endif // UnitTest_fwd_h
