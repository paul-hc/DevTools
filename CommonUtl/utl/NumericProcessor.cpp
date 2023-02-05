
#include "pch.h"
#include "NumericProcessor.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace num
{
	size_t SkipDigitsPos( const str::TStringRange& textRange )
	{
		ASSERT( textRange.InBounds() );
		ASSERT( IsDecimalChar( textRange.GetStartCh() ) );

		bool isHexNumber = str::EqualsIN( textRange.GetStartPtr(), _T("0x"), 2 );
		const TCHAR* pText = textRange.GetText().c_str();
		size_t digitCount = 0;
		size_t startPos = textRange.GetPos().m_start;

		if ( isHexNumber )
			startPos += 2;

		if ( isHexNumber )
		{
			for ( ; IsHexDigitChar( pText[ startPos ] ); ++digitCount )
				++startPos;
		}
		else
		{
			if ( IsSign( pText[ startPos ] ) )
				++startPos;

			for ( ; IsDecimalDigit( pText[ startPos ] ); ++digitCount )
				++startPos;
		}

		if ( 0 == digitCount )
			startPos = textRange.GetPos().m_start;		// number not found, return original start position

		return startPos;
	}

	Radix ParseUnsignedInteger( unsigned int& rOutNumber, size_t& rDigitCount, const TCHAR* pText )
	{
		ASSERT_PTR( pText );

		while ( *pText != _T('\0') && ::_istspace( *pText ) )
			++pText;

		Radix radix = Decimal;

		if ( str::EqualsIN( pText, _T("0x"), 2 ) )
		{
			radix = Hex;
			pText += 2;
		}

		rDigitCount = 0;
		for ( const TCHAR* pDigit = pText; *pDigit != _T('\0') && ( Hex == radix ? IsHexDigitChar( *pDigit ) : IsDecimalChar( *pDigit ) ); ++pDigit )
			++rDigitCount;

		static const TCHAR* s_numFormats[] = { _T("%u"), _T("%x") };

		return 1 == ::_stscanf( pText, s_numFormats[ radix ], &rOutNumber )
			? radix
			: NoNumber;
	}


	str::TStringRange FindNextNumber( const str::TStringRange& textRange )
	{
		ASSERT( textRange.InBounds() );

		str::TStringRange numberRange( textRange );
		Range<size_t>& rNumPos = numberRange.RefPos();

		// skip non-numeric chars
		while ( !numberRange.IsEmpty() && !IsDecimalChar( numberRange.GetStartCh() ) )
			++rNumPos.m_start;

		if ( !numberRange.IsEmpty() )
		{
			size_t endPos = SkipDigitsPos( numberRange );
			return numberRange.MakeLead( endPos );
		}

		return numberRange.MakeTrail( numberRange.GetPos().m_end );		// empty range (at end)
	}

	CSequenceNumber<unsigned int> ExtractNumber( const std::tstring& text ) throws_( CRuntimeException )
	{
		str::TStringRange numberRange = FindNextNumber( text );
		if ( numberRange.IsEmpty() )
			throw CRuntimeException( str::Format( _T("No number found in '%s'"), text.c_str() ) );

		ASSERT( numberRange.InBounds() );

		unsigned int value;
		size_t digitCount;
		Radix radix = ParseUnsignedInteger( value, digitCount, numberRange.GetStartPtr() );
		if ( NoNumber == radix )
			throw CRuntimeException( str::Format( _T("No number found in item '%s'"), numberRange.GetStartPtr() ) );

		CSequenceNumber<unsigned int> parsedNumber( value, _T("%u") );

		switch ( radix )
		{
			case Decimal:
				if ( digitCount > 1 )
					parsedNumber.m_format = str::Format( _T("%%0%du"), digitCount );
				break;
			case Hex:
				parsedNumber.m_format = str::Format( _T("0x%%0%dX"), digitCount );
				break;
		}

		return parsedNumber;
	}

	size_t GenerateNumbersInSequence( std::vector<std::tstring>& rItems, unsigned int startingNumber /*= UINT_MAX*/ ) throws_( CRuntimeException )
	{
		size_t changedCount = 0;

		if ( rItems.size() > 1 )		// multiple items?
		{	// generate a new sequence
			CSequenceNumber<unsigned int> seqNumber( startingNumber, _T("%u") );

			if ( UINT_MAX == startingNumber )
				seqNumber = ExtractNumber( rItems.front() );

			for ( std::vector<std::tstring>::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
				if ( !itItem->empty() )
				{
					str::TStringRange numberRange = FindNextNumber( str::TStringRange( *itItem ) );

					if ( !numberRange.IsEmpty() )
					{
						std::tstring newItem = numberRange.ExtractPrefix() + seqNumber.FormatNextValue() + numberRange.ExtractSuffix();
						if ( newItem != *itItem )
						{
							*itItem = newItem;
							++changedCount;
						}
					}
				}
		}

		return changedCount;
	}
}
