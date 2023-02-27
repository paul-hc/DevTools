#ifndef CodeAlgorithms_h
#define CodeAlgorithms_h
#pragma once

#include "StringParsing.h"
#include "Range.h"


namespace code
{
	template< typename StringT >
	StringT Untabify( const StringT& codeText, size_t tabSize = 4 )		// works with both "\n" and "\r\n" line ends
	{
		REQUIRE( tabSize != 0 );

		StringT outcome;
		size_t tabCount = std::count( codeText.begin(), codeText.end(), '\t' );

		outcome.reserve( codeText.size() + tabCount * tabSize );

		Range<size_t> outLine( 0 );			// output line relative positions (are reset for each line)

		for ( size_t pos = 0, length = codeText.length(); pos != length; ++pos )
		{
			if ( '\t' == codeText[pos] )
			{	// fill with spaces, depending on the current tab position in the current line
				for ( size_t spaceFillCount = tabSize - ( outLine.GetSpan<size_t>() % tabSize ); spaceFillCount-- != 0; ++outLine.m_end )
					outcome.push_back( ' ' );
			}
			else
			{
				outcome.push_back( codeText[pos] );		// note: if '\r' is encountered, it will be copied as usual

				if ( '\n' == codeText[pos] )
					outLine.SetEmptyRange( 0 );			// reset out positions to beginning of a new line
				else
					++outLine.m_end;
			}
		}
		return outcome;
	}
}


#endif // CodeAlgorithms_h
