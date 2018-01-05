
#include "stdafx.h"
#include "InputOutput.h"
#include "utl/StringUtilities.h"
#include "utl/RuntimeException.h"
#include <iostream>


namespace io
{
	char InputUserKey( bool skipWhitespace /*= true*/ )
	{
		std::string userKeyAnswer;

		if ( !skipWhitespace )
			std::cin.unsetf( std::ios::skipws );			// count whitespace as regular character
		std::cin >> userKeyAnswer;
		if ( !skipWhitespace )
			std::cin.setf( std::ios::skipws );			// restore

		size_t keyPos = userKeyAnswer.find_first_not_of( " \t" );
		return keyPos != std::tstring::npos ? (char)toupper( userKeyAnswer[ keyPos ] ) : '\0';
	}
}
