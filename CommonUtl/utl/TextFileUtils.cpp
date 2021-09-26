
#include "stdafx.h"
#include "TextFileUtils.h"
#include "Path.h"
#include "EnumTags.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TextFileUtils.hxx"


namespace io
{
	namespace bin
	{
		// buffer I/O to/from text file

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::in | std::ios_base::binary );
			io::CheckOpenForReading( ifs, srcFilePath );

			rBuffer.assign( std::istreambuf_iterator<char>( ifs ),  std::istreambuf_iterator<char>() );		// verbatim buffer of chars
		}
	}
}


namespace io
{
	// line I/O

	template<>
	void ReadTextLine< std::string >( std::istream& is, std::string& rLineUtf8 )
	{
		std::getline( is, rLineUtf8 );
	}

	template<>
	void ReadTextLine< std::wstring >( std::istream& is, std::wstring& rLineWide )
	{
		std::string lineUtf8;
		std::getline( is, lineUtf8 );

		rLineWide = str::FromUtf8( lineUtf8.c_str() );
	}


	bool WriteLineEnd( std::ostream& os, size_t& rLinePos )
	{	// prepend a line end if not the first line
		if ( 0 == rLinePos++ )
			return false;			// skip newline for the first added line

		os << std::endl;
		return true;
	}

	template<>
	void WriteTextLine< std::string >( std::ostream& os, const std::string& lineUtf8, size_t* pLinePos /*= NULL*/ )
	{
		if ( pLinePos != NULL )
			WriteLineEnd( os, *pLinePos );

		os << lineUtf8.c_str();
	}

	template<>
	void WriteTextLine< std::wstring >( std::ostream& os, const std::wstring& lineWide, size_t* pLinePos /*= NULL*/ )
	{
		if ( pLinePos != NULL )
			WriteLineEnd( os, *pLinePos );

		os << str::ToUtf8( lineWide.c_str() );
	}
}
