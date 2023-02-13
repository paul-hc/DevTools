#ifndef CodeParser_h
#define CodeParser_h
#pragma once

#include "utl/CodeParsing.h"
#include "utl/Range.h"
#include "TokenRange.h"


namespace pvt
{
	template< typename IteratorT >
	inline int Distance( IteratorT itFirst, IteratorT itLast ) { return static_cast<int>( std::distance( itFirst, itLast ) ); }


	// convert from iterator range to TokenRange:

	template< typename IteratorT >
	inline TokenRange MakeTokenRange( const Range<IteratorT>& itRange, IteratorT itBegin )
	{
		return TokenRange( str::MakePosRange( itRange, itBegin ) );
	}

	template< typename ContainerT >
	inline TokenRange MakeFwdTokenRange( const Range<typename ContainerT::const_reverse_iterator>& itRange, const ContainerT& items )
	{
		return TokenRange( str::MakeFwdPosRange( itRange, items ) );
	}
}


class CCodeParser
{
public:
	CCodeParser( const code::CLanguage<TCHAR>& lang ) : m_lang( lang ) {}

	bool FindArgumentList( TokenRange* pOutArgList, const std::tstring& proto, size_t offset = 0 ) const;
private:
	const code::CLanguage<TCHAR>& m_lang;
};


#endif // CodeParser_h
