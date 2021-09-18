
#include "stdafx.h"
#include "TextFileUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// line streaming template code

namespace utl
{
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


	template<>
	void ReadText< std::string >( std::istream& is, std::string& rText )
	{
		rText = std::string( std::istreambuf_iterator<char>( is ),  std::istreambuf_iterator<char>() );
	}

	template<>
	void ReadText< std::wstring >( std::istream& is, std::wstring& rText )
	{
		std::string textUtf8;
		ReadText( is, textUtf8 );

		rText = str::FromUtf8( textUtf8.c_str() );
	}


	template<>
	void WriteText< std::string >( std::ostream& os, const std::string& textUtf8 )
	{
		os << textUtf8.c_str();
	}


	template<>
	void WriteText< std::wstring >( std::ostream& os, const std::wstring& textWide )
	{
		os << str::ToUtf8( textWide.c_str() );
	}
}
