
#include "stdafx.h"
#include "HexDump.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	void HexDump( std::ostream& os, const std::string& textPath, size_t rowByteCount /*= DefaultRowByteCount*/ )
	{	// dump contents of filename to stdout in hex
		std::fstream is( textPath.c_str(), std::ios::in | std::ios::binary );

		if ( !is.is_open() )
			throw std::runtime_error( std::string("Cannot open ") + textPath + " for reading" );

		static const char s_unprintableCh = '.';
		static const char s_blankCh = ' ';
		static const char s_columnSep[] = "  ";

		std::vector< char > inputRowBuff( rowByteCount );
		std::string charRow;

		for ( size_t i; !is.eof(); )
		{
			inputRowBuff.assign( rowByteCount, '\0' );

			is.read( &inputRowBuff[ 0 ], rowByteCount );
			inputRowBuff.resize( is.gcount() );		// cut to the actual read chars

			if ( !inputRowBuff.empty() )
			{
				charRow.clear();

				for ( i = 0; i != rowByteCount; ++i )
					if ( i < inputRowBuff.size() )
					{
						char inChar = inputRowBuff[ i ];

						charRow.push_back( inChar >= ' ' ? inChar : s_unprintableCh );
						os << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (int)(unsigned char)inChar << ' ';
					}
					else
						os << "   ";		// 2 digits + 1 space

				os << s_columnSep << charRow << std::endl;
			}
		}
		is.close();

		/* Sample outputs:
			Hex Dump of wcHello.txt - note that output is ANSI chars:
			48 65 6C 6C 6F 20 57 6F 72 6C 64 00 00 00 00 00   Hello World.....

			Hex Dump of wwHello.txt - note that output is wchar_t chars:
			48 00 65 00 6c 00 6c 00 6f 00 20 00 57 00 6f 00   H.e.l.l.o. .W.o.
			72 00 6c 00 64 00                                 r.l.d.
		*/
	}
}
