
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/TextFileIoTests.h"
#include "Path.h"
#include "StringUtilities.h"
#include "EncodedFileBuffer.h"
#include "TextEncoding.h"
#include "TextFileIo.h"
#include "IoBin.h"
#include "EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TextFileIo.hxx"


std::wostream& operator<<( std::wostream& os, const std::vector< char >& buffer )
{
	os << L"hex: ";		// separator
	for ( size_t pos = 0; pos != buffer.size(); ++pos )
	{
		if ( pos != 0 )
			os << L' ';		// separator

		os << str::Format( L"%02X", (unsigned char)buffer[ pos ] );
	}
	return os;
}

template< typename StringT >
std::wostream& operator<<( std::wostream& os, const std::vector< StringT >& strings )
{
	os << L"{";
	size_t linePos = 0;
	for ( typename std::vector< StringT >::const_iterator itString = strings.begin(); itString != strings.end(); ++itString )
	{
		if ( linePos++ != 0 )
			os << L' ';

		os << L'"';
		os << *itString;
		os << L'"';
	}
	os << L"}";
	return os;
}


namespace ut
{
	template< typename CharType >
	std::vector< CharType >& AssignText( std::vector< CharType >& rBuffer, const CharType* pText, size_t textSize = std::string::npos )
	{
		if ( pText != NULL )
		{
			if ( std::string::npos == textSize )
				textSize = str::GetLength( pText );

			rBuffer.assign( pText, pText + textSize );		// excluding the EOS
		}
		else
			rBuffer.clear();

		return rBuffer;
	}

	std::tstring FormatTextFilename( fs::Encoding encoding, const TCHAR fmt[] = _T("tf_%s.txt") )
	{
		return str::Format( fmt, fs::GetTags_Encoding().FormatKey( encoding ).c_str() );
	}

	const std::vector< std::string >& ToUtf8Lines( const std::vector< std::wstring >& lines )
	{
		static std::vector< std::string > s_linesUtf8;

		s_linesUtf8.clear();
		for ( std::vector< std::wstring >::const_iterator itLine = lines.begin(); itLine != lines.end(); ++itLine )
			s_linesUtf8.push_back( str::ToUtf8( itLine->c_str() ) );

		return s_linesUtf8;
	}

	const std::vector< std::wstring >& FromUtf8Lines( const std::vector< std::string >& lines )
	{
		static std::vector< std::wstring > s_linesWide;

		s_linesWide.clear();
		for ( std::vector< std::string >::const_iterator itLine = lines.begin(); itLine != lines.end(); ++itLine )
			s_linesWide.push_back( str::FromUtf8( itLine->c_str() ) );

		return s_linesWide;
	}
}


namespace ut
{
	// test implementation
	void test_WriteRead_BinBuffer( fs::Encoding encoding, const fs::CPath& textPath, const std::string& content, const char pExpected[], size_t expectedSize = std::string::npos );

	void test_WriteRead( fs::Encoding encoding );
	void test_WriteReadContent( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent );
	void test_WriteReadFlat( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent );
	void test_WriteReadLines( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent );

	void test_WriteReadLines_Stream( fs::Encoding encoding );
	void test_WriteReadLines_StreamImpl( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent );

	void test_WriteRead_Rewind( fs::Encoding encoding );
	void test_WriteRead_Rewind( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent );

	void test_WriteParseLines_A( fs::Encoding encoding );
	void test_WriteParseLines_W( fs::Encoding encoding );

	template< typename StringT >
	void test_ParseSaveVerbatimContent( fs::Encoding encoding, const StringT& content );
}


CTextFileIoTests::CTextFileIoTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTextFileIoTests& CTextFileIoTests::Instance( void )
{
	static CTextFileIoTests s_testCase;
	return s_testCase;
}


void CTextFileIoTests::TestByteOrderMark( void )
{
	{
		fs::CByteOrderMark bomAnsi( fs::ANSI_UTF8 );

		ASSERT( bomAnsi.IsEmpty() );
	}
	{
		fs::CByteOrderMark bomUTF8( fs::UTF8_bom );

		ASSERT_EQUAL( 3, bomUTF8.Get().size() );
		ASSERT_EQUAL( 0xEF, bomUTF8.Get()[ 0 ] );
		ASSERT_EQUAL( 0xBB, bomUTF8.Get()[ 1 ] );
		ASSERT_EQUAL( 0xBF, bomUTF8.Get()[ 2 ] );
	}
	{
		fs::CByteOrderMark bomUTF16_LE( fs::UTF16_LE_bom );

		ASSERT_EQUAL( 2, bomUTF16_LE.Get().size() );
		ASSERT_EQUAL( 0xFF, bomUTF16_LE.Get()[ 0 ] );
		ASSERT_EQUAL( 0xFE, bomUTF16_LE.Get()[ 1 ] );
	}
	{
		fs::CByteOrderMark bomUTF16_be( fs::UTF16_be_bom );

		ASSERT_EQUAL( 2, bomUTF16_be.Get().size() );
		ASSERT_EQUAL( 0xFE, bomUTF16_be.Get()[ 0 ] );
		ASSERT_EQUAL( 0xFF, bomUTF16_be.Get()[ 1 ] );
	}
	{
		fs::CByteOrderMark bomUTF32_LE( fs::UTF32_LE_bom );

		ASSERT_EQUAL( 4, bomUTF32_LE.Get().size() );
		ASSERT_EQUAL( 0xFF, bomUTF32_LE.Get()[ 0 ] );
		ASSERT_EQUAL( 0xFE, bomUTF32_LE.Get()[ 1 ] );
		ASSERT_EQUAL( 0x00, bomUTF32_LE.Get()[ 2 ] );
		ASSERT_EQUAL( 0x00, bomUTF32_LE.Get()[ 3 ] );
	}
	{
		fs::CByteOrderMark bomUTF32_be( fs::UTF32_be_bom );

		ASSERT_EQUAL( 4, bomUTF32_be.Get().size() );
		ASSERT_EQUAL( 0x00, bomUTF32_be.Get()[ 0 ] );
		ASSERT_EQUAL( 0x00, bomUTF32_be.Get()[ 1 ] );
		ASSERT_EQUAL( 0xFE, bomUTF32_be.Get()[ 2 ] );
		ASSERT_EQUAL( 0xFF, bomUTF32_be.Get()[ 3 ] );
	}
}

void CTextFileIoTests::TestWriteRead_BinBuffer( void )
{
	std::string content;
	{
		fs::Encoding encoding = fs::ANSI_UTF8;

		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, NULL );

		// single-line file
		content = "xyz";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "xyz" );

		// check multiple-lines file
		content = "A1\nB2";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "A1\r\nB2" );
	}
	{
		fs::Encoding encoding = fs::UTF8_bom;

		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "\xEF\xBB\xBF" );

		// single-line file
		content = "xyz";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "\xEF\xBB\xBF" "xyz" );

		// check multiple-lines file
		content = "A1\nB2";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "\xEF\xBB\xBF" "A1\r\nB2" );
	}
	{
		fs::Encoding encoding = fs::UTF16_LE_bom;

		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "\xFF\xFE" );

		// single-line file
		content = "xyz";
		static const char s_slExpected[] = { '\xFF', '\xFE', 'x', 0, 'y', 0, 'z', 0 };
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, s_slExpected, COUNT_OF( s_slExpected ) );

		// check multiple-lines file
		content = "A1\nB2";
		static const char s_mlExpected[] = { '\xFF', '\xFE', 'A', 0, '1', 0, '\r', 0, '\n', 0, 'B', 0, '2', 0 };
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, s_mlExpected, COUNT_OF( s_mlExpected ) );
	}
	{
		fs::Encoding encoding = fs::UTF16_be_bom;

		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, "\xFE\xFF" );

		// single-line file
		content = "xyz";
		static const char s_slExpected[] = { '\xFE', '\xFF', 0, 'x', 0, 'y', 0, 'z' };
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, s_slExpected, COUNT_OF( s_slExpected ) );

		// check multiple-lines file
		content = "A1\nB2";
		static const char s_mlExpected[] = { '\xFE', '\xFF', 0, 'A', 0, '1', 0, '\r', 0, '\n', 0, 'B', 0, '2' };
		ut::test_WriteRead_BinBuffer( encoding, textPath, content, s_mlExpected, COUNT_OF( s_mlExpected ) );
	}
}

	void ut::test_WriteRead_BinBuffer( fs::Encoding encoding, const fs::CPath& textPath, const std::string& content, const char pExpected[], size_t expectedSize /*= std::string::npos*/ )
	{
		std::vector< char > expectedBuffer;
		AssignText( expectedBuffer, pExpected, expectedSize );

		io::WriteStringToFile( textPath, content, encoding );

		// test input binary buffer
		std::vector< char > inBuffer;
		io::bin::ReadAllFromFile( inBuffer, textPath );
		ASSERT_EQUAL( expectedBuffer, inBuffer );

		// test input content
		std::string inContent;
		ASSERT_EQUAL( encoding, io::ReadStringFromFile( inContent, textPath ) );
		ASSERT_EQUAL( content, inContent );
	}


void CTextFileIoTests::TestWriteRead_Contents( void )
{
	ut::test_WriteRead( fs::ANSI_UTF8 );
	ut::test_WriteRead( fs::UTF8_bom );
	ut::test_WriteRead( fs::UTF16_LE_bom );
	ut::test_WriteRead( fs::UTF16_be_bom );
}

	void ut::test_WriteRead( fs::Encoding encoding )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		ut::test_WriteReadContent( encoding, textPath, L"" );				// empty file
		ut::test_WriteReadContent( encoding, textPath, L"xyz_αβγ" );		// single-line file
		ut::test_WriteReadContent( encoding, textPath, L"\n" );				// 2-lines file: empty
		ut::test_WriteReadContent( encoding, textPath, L"\nAα1" );			// 2-lines file: first empty
		ut::test_WriteReadContent( encoding, textPath, L"Aα1\n" );			// 2-lines file: second empty
		ut::test_WriteReadContent( encoding, textPath, L"ABC\nαβγ\nXYZ" );	// multi-line file
	}

	void ut::test_WriteReadContent( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent )
	{
		ut::test_WriteReadFlat( encoding, textPath, pContent );
		ut::test_WriteReadLines( encoding, textPath, pContent );
	}

	void ut::test_WriteReadFlat( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent )
	{
		{	// NARROW
			std::string content( str::ToUtf8( pContent ) );
			io::WriteStringToFile( textPath, content, encoding );

			{
				std::string outText;
				ASSERT_EQUAL( encoding, io::ReadStringFromFile( outText, textPath ) );
				ASSERT_EQUAL( content, outText );
			}

			{	// cross-read WIDE
				std::wstring outText;
				ASSERT_EQUAL( encoding, io::ReadStringFromFile( outText, textPath ) );
				ASSERT_EQUAL( content, str::ToUtf8( outText.c_str() ) );
			}
		}
		{	// WIDE
			std::wstring content( pContent );
			io::WriteStringToFile( textPath, content, encoding );

			{
				std::wstring outText;
				ASSERT_EQUAL( encoding, io::ReadStringFromFile( outText, textPath ) );
				ASSERT_EQUAL( content, outText );
			}

			{	// cross-read NARROW
				std::string outText;
				ASSERT_EQUAL( encoding, io::ReadStringFromFile( outText, textPath ) );
				ASSERT_EQUAL( content, str::FromUtf8( outText.c_str() ) );
			}
		}
	}

	void ut::test_WriteReadLines( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent )
	{
		{	// NARROW
			std::string content( str::ToUtf8( pContent ) );
			std::vector< std::string > contentLines;
			str::Split( contentLines, str::ToUtf8( pContent ).c_str(), "\n" );

			io::WriteLinesToFile( textPath, contentLines, encoding );

			{
				std::vector< std::string > outLines;
				ASSERT_EQUAL( encoding, io::ReadLinesFromFile( outLines, textPath ) );
				ASSERT_EQUAL( contentLines, outLines );
			}

			{	// cross-read lines WIDE
				std::vector< std::wstring > outLines;
				ASSERT_EQUAL( encoding, io::ReadLinesFromFile( outLines, textPath ) );
				ASSERT_EQUAL( contentLines, ut::ToUtf8Lines( outLines ) );
			}
		}
		{	// WIDE
			std::wstring content( pContent );
			std::vector< std::wstring > contentLines;
			str::Split( contentLines, pContent, L"\n" );

			io::WriteLinesToFile( textPath, contentLines, encoding );

			std::vector< std::wstring > outLines;
			ASSERT_EQUAL( encoding, io::ReadLinesFromFile( outLines, textPath ) );
			ASSERT_EQUAL( contentLines, outLines );

			{	// cross-read lines NARROW
				std::vector< std::string > outLines;
				ASSERT_EQUAL( encoding, io::ReadLinesFromFile( outLines, textPath ) );
				ASSERT_EQUAL( contentLines, ut::FromUtf8Lines( outLines ) );
			}
		}
	}


void CTextFileIoTests::TestWriteReadLines_StreamGetLine( void )
{
	ut::test_WriteReadLines_Stream( fs::ANSI_UTF8 );
	ut::test_WriteReadLines_Stream( fs::UTF8_bom );
	ut::test_WriteReadLines_Stream( fs::UTF16_LE_bom );
	ut::test_WriteReadLines_Stream( fs::UTF16_be_bom );
}

	void ut::test_WriteReadLines_Stream( fs::Encoding encoding )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"" );				// empty file
		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"xyz_αβγ" );		// single-line file
		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"\n" );			// 2-lines file: empty
		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"\nAα1" );			// 2-lines file: first empty
		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"Aα1\n" );			// 2-lines file: second empty
		ut::test_WriteReadLines_StreamImpl( encoding, textPath, L"ABC\nαβγ\nXYZ" );	// multi-line file
	}

	void ut::test_WriteReadLines_StreamImpl( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent )
	{
		std::string content( str::ToUtf8( pContent ) );
		std::vector< std::string > contentLines;
		str::Split( contentLines, str::ToUtf8( pContent ).c_str(), "\n" );

		io::WriteLinesToFile( textPath, contentLines, encoding );

		if ( fs::GetCharByteCount( encoding ) == sizeof(char) )
		{	// NARROW stream
			std::istream is( NULL );
			io::CEncodedStreamFileBuffer<char> fileBuffer( is, encoding );
			fileBuffer.Open( textPath, std::ios_base::in );
			{
				std::vector< std::string > outLines;

				for ( std::string line; io::GetLine( is, line ); )
					outLines.push_back( line );

				ASSERT_EQUAL( contentLines, outLines );
			}

			{	// cross-read lines WIDE
				fileBuffer.Rewind();
				std::vector< std::wstring > outLines;

				for ( std::wstring line; io::GetLine( is, line ); )
					outLines.push_back( line );

				ASSERT_EQUAL( contentLines, ut::ToUtf8Lines( outLines ) );
			}
		}
		else if ( fs::GetCharByteCount( encoding ) == sizeof(wchar_t) )
		{	// WIDE
			std::wistream is( NULL );
			io::CEncodedStreamFileBuffer<wchar_t> fileBuffer( is, encoding );
			fileBuffer.Open( textPath, std::ios_base::in );
			{
				std::vector< std::wstring > outLines;

				for ( std::wstring line; io::GetLine( is, line ); )
					outLines.push_back( line );

				ASSERT_EQUAL( contentLines, ut::ToUtf8Lines( outLines ) );
			}

			{	// cross-read lines NARROW
				fileBuffer.Rewind();
				std::vector< std::string > outLines;

				for ( std::string line; io::GetLine( is, line ); )
					outLines.push_back( line );

				ASSERT_EQUAL( contentLines, outLines );
			}
		}
	}


void CTextFileIoTests::TestWriteReadLines_Rewind( void )
{
	ut::test_WriteRead_Rewind( fs::ANSI_UTF8 );
	ut::test_WriteRead_Rewind( fs::UTF8_bom );
	ut::test_WriteRead_Rewind( fs::UTF16_LE_bom );
	ut::test_WriteRead_Rewind( fs::UTF16_be_bom );
}

	void ut::test_WriteRead_Rewind( fs::Encoding encoding )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		ut::test_WriteRead_Rewind( encoding, textPath, L"" );				// empty file
		ut::test_WriteRead_Rewind( encoding, textPath, L"xyz_αβγ" );		// single-line file
		ut::test_WriteRead_Rewind( encoding, textPath, L"\n" );				// 2-lines file: empty
		ut::test_WriteRead_Rewind( encoding, textPath, L"\nAα1" );			// 2-lines file: first empty
		ut::test_WriteRead_Rewind( encoding, textPath, L"Aα1\n" );			// 2-lines file: second empty
		ut::test_WriteRead_Rewind( encoding, textPath, L"ABC\nαβγ\nXYZ" );	// multi-line file
	}

	void ut::test_WriteRead_Rewind( fs::Encoding encoding, const fs::CPath& textPath, const wchar_t* pContent )
	{
		std::wstring wcontent( pContent );
		io::WriteStringToFile( textPath, wcontent, encoding );

		if ( fs::GetCharByteCount( encoding ) == sizeof(char) )
		{	// NARROW encoding
			std::string content( str::ToUtf8( pContent ) );

			io::CEncodedFileBuffer<char> inFile( encoding );
			inFile.Open( textPath, std::ios_base::in );

			std::string outText;

			inFile.GetString( outText );
			ASSERT_EQUAL( content, outText );

			inFile.GetString( outText );
			ASSERT( outText.empty() );			// reached EOF, no more text

			inFile.Rewind();					// go back to the start of text
			inFile.GetString( outText );
			ASSERT_EQUAL( content, outText );
		}
		else if ( fs::GetCharByteCount( encoding ) == sizeof(wchar_t) )
		{	// WIDE encoding
			io::CEncodedFileBuffer<wchar_t> inFile( encoding );
			inFile.Open( textPath, std::ios_base::in );

			std::wstring outText;

			inFile.GetString( outText );
			ASSERT_EQUAL( wcontent, outText );

			inFile.GetString( outText );
			ASSERT( outText.empty() );			// reached EOF, no more text

			inFile.Rewind();
			inFile.GetString( outText );
			ASSERT_EQUAL( wcontent, outText );
		}
	}


void CTextFileIoTests::TestWriteParseLines( void )
{
	ut::test_WriteParseLines_A( fs::ANSI_UTF8 );
	ut::test_WriteParseLines_A( fs::UTF8_bom );
	ut::test_WriteParseLines_A( fs::UTF16_LE_bom );
	ut::test_WriteParseLines_A( fs::UTF16_be_bom );

	ut::test_WriteParseLines_W( fs::ANSI_UTF8 );
	ut::test_WriteParseLines_W( fs::UTF8_bom );
	ut::test_WriteParseLines_W( fs::UTF16_LE_bom );
	ut::test_WriteParseLines_W( fs::UTF16_be_bom );
}

	void ut::test_WriteParseLines_A( fs::Encoding encoding )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		io::CTextFileParser< std::string > narrowParser;
		const std::vector< std::string >& parsedLines = narrowParser.GetParsedLines();

		// check empty file
		io::WriteStringToFile( textPath, std::string(), encoding );
		narrowParser.ParseFile( textPath );
		ASSERT( parsedLines.empty() );

		// check single-line file (no line-end)
		static const std::string s_singleText( "ABC" );
		io::WriteStringToFile( textPath, s_singleText, encoding );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 1, parsedLines.size() );
		ASSERT_EQUAL( s_singleText, parsedLines[ 0 ] );

		// check 2-lines file (1 line-end)
		io::WriteStringToFile( textPath, s_singleText + "\n", encoding );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 2, parsedLines.size() );
		ASSERT_EQUAL( s_singleText, parsedLines[ 0 ] );
		ASSERT_EQUAL( "", parsedLines[ 1 ] );

		// check multiple-lines file
		io::WriteStringToFile( textPath, std::string( "A1\nB2\nC3\nD4" ), encoding );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 4, parsedLines.size() );
		ASSERT_EQUAL( "A1", parsedLines[ 0 ] );
		ASSERT_EQUAL( "B2", parsedLines[ 1 ] );
		ASSERT_EQUAL( "C3", parsedLines[ 2 ] );
		ASSERT_EQUAL( "D4", parsedLines[ 3 ] );
	}

	void ut::test_WriteParseLines_W( fs::Encoding encoding )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		io::CTextFileParser< std::wstring > wideParser;
		const std::vector< std::wstring >& parsedLines = wideParser.GetParsedLines();

		// check empty file
		io::WriteStringToFile( textPath, std::wstring(), encoding );
		wideParser.ParseFile( textPath );
		ASSERT( parsedLines.empty() );

		// check single-line file (no line-end)
		static const std::wstring s_singleText( L"ABC" );
		io::WriteStringToFile( textPath, s_singleText, encoding );
		wideParser.ParseFile( textPath );
		ASSERT_EQUAL( 1, parsedLines.size() );
		ASSERT_EQUAL( s_singleText, parsedLines[ 0 ] );

		// check 2-lines file (1 line-end)
		io::WriteStringToFile( textPath, s_singleText + L"\n", encoding );
		wideParser.ParseFile( textPath );
		ASSERT_EQUAL( 2, parsedLines.size() );
		ASSERT_EQUAL( s_singleText, parsedLines[ 0 ] );
		ASSERT_EQUAL( L"", parsedLines[ 1 ] );

		// check multiple-lines file
		io::WriteStringToFile( textPath, std::wstring( L"A1\nB2\nC3\nD4" ), encoding );
		wideParser.ParseFile( textPath );
		ASSERT_EQUAL( 4, parsedLines.size() );
		ASSERT_EQUAL( L"A1", parsedLines[ 0 ] );
		ASSERT_EQUAL( L"B2", parsedLines[ 1 ] );
		ASSERT_EQUAL( L"C3", parsedLines[ 2 ] );
		ASSERT_EQUAL( L"D4", parsedLines[ 3 ] );
	}

void CTextFileIoTests::TestParseSaveVerbatimContent( void )
{
	static const char* s_contents[] =
	{
		"",
		"ABC",
		"\n",
		"\nABC",
		"ABC\n",
		"Line 1\nLine 2\nLine 3\nLine 4"
	};

	for ( size_t i = 0; i != COUNT_OF( s_contents ); ++i )
	{
		std::string content = s_contents[ i ];

		ut::test_ParseSaveVerbatimContent( fs::ANSI_UTF8, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF8_bom, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF16_LE_bom, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF16_be_bom, content );
	}

	for ( size_t i = 0; i != COUNT_OF( s_contents ); ++i )
	{
		std::wstring content = str::FromUtf8( s_contents[ i ] );

		ut::test_ParseSaveVerbatimContent( fs::ANSI_UTF8, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF8_bom, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF16_LE_bom, content );
		ut::test_ParseSaveVerbatimContent( fs::UTF16_be_bom, content );
	}
}

	template< typename StringT >
	void ut::test_ParseSaveVerbatimContent( fs::Encoding encoding, const StringT& content )
	{
		ut::CTempFilePool pool( ut::FormatTextFilename( encoding ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

		io::WriteStringToFile( textPath, content, encoding );

		{	// do the round-trip: parse & save
			io::CTextFileParser< StringT > parser;

			parser.ParseFile( textPath );
			io::WriteLinesToFile( textPath, parser.GetParsedLines(), parser.GetEncoding() );		// save all lines
		}

		StringT inContent;
		ASSERT_EQUAL( encoding, io::ReadStringFromFile( inContent, textPath ) );
		ASSERT_EQUAL( content, inContent );
	}


void CTextFileIoTests::Run( void )
{
	__super::Run();

	TestByteOrderMark();
	TestWriteRead_BinBuffer();
	TestWriteRead_Contents();
	TestWriteReadLines_StreamGetLine();
	TestWriteReadLines_Rewind();
	TestWriteParseLines();
	TestParseSaveVerbatimContent();
}


#endif //USE_UT
