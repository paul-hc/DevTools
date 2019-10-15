#ifndef NumericProcessor_h
#define NumericProcessor_h
#pragma once

#include "StringRange.h"


namespace num
{
	inline bool IsDecimalDigit( TCHAR chr ) { return str::CharTraits::IsDigit( chr ); }
	inline bool IsSign( TCHAR chr ) { return '-' == chr || '+' == chr; }
	inline bool IsDecimalChar( TCHAR chr ) { return IsDecimalDigit( chr ) || IsSign( chr ); }

	inline bool IsHexDigitChar( TCHAR chr ) { return ::_istxdigit( chr ) != FALSE; }

	void SkipDigit( str::TStringRange& rTextRange );

	enum Radix { Decimal, Hex, NoNumber };

	Radix ParseUnsignedInteger( unsigned int& rOutNumber, const TCHAR* pText );


	template< typename IntegralType >
	struct CSequenceNumber
	{
		CSequenceNumber( IntegralType number, const std::tstring& format ) : m_number( number ), m_format( format ) {}
		CSequenceNumber( const CSequenceNumber& source ) : m_number( source.m_number ), m_format( source.m_format ) {}

		std::tstring FormatNextValue( void ) { return str::Format( m_format.c_str(), m_number++ ); }
	public:
		IntegralType m_number;
		std::tstring m_format;
	};


	str::TStringRange FindNextNumber( const str::TStringRange& textRange );
	inline str::TStringRange FindNextNumber( const std::tstring& text ) { return FindNextNumber( str::TStringRange( text ) ); }

	CSequenceNumber< unsigned int > ExtractNumber( const std::tstring& text ) throws_( CRuntimeException );

	size_t GenerateNumbersInSequence( std::vector< std::tstring >& rItems, unsigned int startingNumber = UINT_MAX ) throws_( CRuntimeException );
}


#endif // NumericProcessor_h
