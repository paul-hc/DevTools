#ifndef StringBase_hxx
#define StringBase_hxx
#pragma once


namespace str
{
	template< typename StringT, typename ValueT >
	StringT ValueToString( const ValueT& value )
	{
		std::basic_ostringstream< typename StringT::value_type > oss;
		oss << value;
		return oss.str();
	}
}


#endif // StringBase_hxx
