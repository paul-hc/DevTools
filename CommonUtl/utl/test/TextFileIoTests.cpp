
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/TextFileIoTests.h"
#include "Path.h"
#include "StringUtilities.h"
#include "TextEncoding.h"
#include "TextFileUtils.h"

#define new DEBUG_NEW

#include "TextFileUtils.hxx"


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

#define LAST_OF( array ) ( END_OF( array ) - 1 )	// excluding the EOS

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

	// test implementation
	void test_TextFile_Narrow( fs::Encoding encoding, const fs::CPath& textPath, const std::string& content, const char pExpected[], size_t expectedSize = std::string::npos );
	void test_FileBom_NarrowIo( fs::Encoding encoding, const ut::CTempFilePool& pool );
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
		fs::CByteOrderMark bomAnsi( fs::ANSI );

		ASSERT( !bomAnsi.HasBom() );
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

void CTextFileIoTests::TestTextFileWithEncoding( void )
{
	std::string content;
	{
		fs::Encoding encoding = fs::ANSI;

		ut::CTempFilePool pool( str::Format( _T("tf_%s.txt"), fs::GetTags_Encoding().FormatKey( encoding ).c_str() ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_TextFile_Narrow( encoding, textPath, content, NULL );

		// single-line file
		content = "xyz";
		ut::test_TextFile_Narrow( encoding, textPath, content, "xyz" );

		// check multiple-lines file
		content = "A1\nB2";
		ut::test_TextFile_Narrow( encoding, textPath, content, "A1\r\nB2" );
	}
	{
		fs::Encoding encoding = fs::UTF8_bom;

		ut::CTempFilePool pool( str::Format( _T("tf_%s.txt"), fs::GetTags_Encoding().FormatKey( encoding ).c_str() ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_TextFile_Narrow( encoding, textPath, content, "\xEF\xBB\xBF" );

		// single-line file
		content = "xyz";
		ut::test_TextFile_Narrow( encoding, textPath, content, "\xEF\xBB\xBF" "xyz" );

		// check multiple-lines file
		content = "A1\nB2";
		ut::test_TextFile_Narrow( encoding, textPath, content, "\xEF\xBB\xBF" "A1\r\nB2" );
	}
	{
		fs::Encoding encoding = fs::UTF16_LE_bom;

		ut::CTempFilePool pool( str::Format( _T("tf_%s.txt"), fs::GetTags_Encoding().FormatKey( encoding ).c_str() ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_TextFile_Narrow( encoding, textPath, content, "\xFF\xFE" );

		// single-line file
		content = "xyz";
		static const char s_slExpected[] = { '\xFF', '\xFE', 'x', 0, 'y', 0, 'z', 0 };
		ut::test_TextFile_Narrow( encoding, textPath, content, s_slExpected, COUNT_OF( s_slExpected ) );

		// check multiple-lines file
		content = "A1\nB2";
		static const char s_mlExpected[] = { '\xFF', '\xFE', 'A', 0, '1', 0, '\r', 0, '\n', 0, 'B', 0, '2', 0 };
		ut::test_TextFile_Narrow( encoding, textPath, content, s_mlExpected, COUNT_OF( s_mlExpected ) );
	}
	{
		fs::Encoding encoding = fs::UTF16_be_bom;

		ut::CTempFilePool pool( str::Format( _T("tf_%s.txt"), fs::GetTags_Encoding().FormatKey( encoding ).c_str() ).c_str() );
		const fs::CPath& textPath = pool.GetFilePaths().front();

		// check empty file
		content = "";
		ut::test_TextFile_Narrow( encoding, textPath, content, "\xFE\xFF" );

		// single-line file
		content = "xyz";
		static const char s_slExpected[] = { '\xFE', '\xFF', 0, 'x', 0, 'y', 0, 'z' };
		ut::test_TextFile_Narrow( encoding, textPath, content, s_slExpected, COUNT_OF( s_slExpected ) );

		// check multiple-lines file
		content = "A1\nB2";
		static const char s_mlExpected[] = { '\xFE', '\xFF', 0, 'A', 0, '1', 0, '\r', 0, '\n', 0, 'B', 0, '2' };
		ut::test_TextFile_Narrow( encoding, textPath, content, s_mlExpected, COUNT_OF( s_mlExpected ) );
	}
}

void ut::test_TextFile_Narrow( fs::Encoding encoding, const fs::CPath& textPath, const std::string& content, const char pExpected[], size_t expectedSize /*= std::string::npos*/ )
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


void CTextFileIoTests::TestFileBom_NarrowIo( void )
{
	ut::CTempFilePool pool( _T("tf_ANSI.txt|tf_UTF8.txt|tf_UTF16_LE.txt|tf_UTF16_le.txt|tf_UTF32_LE.txt|tf_UTF32_be.txt") );

	ut::test_FileBom_NarrowIo( fs::ANSI, pool );
	ut::test_FileBom_NarrowIo( fs::UTF8_bom, pool );
	ut::test_FileBom_NarrowIo( fs::UTF16_LE_bom, pool );
	ut::test_FileBom_NarrowIo( fs::UTF16_be_bom, pool );
}

void ut::test_FileBom_NarrowIo( fs::Encoding encoding, const ut::CTempFilePool& pool )
{
	const fs::CPath& textPath = pool.GetFilePaths()[ encoding ];

	CTextFileParser< std::string > narrowParser;
	const std::vector< std::string >& narrowLines = narrowParser.GetParsedLines();

	// check empty file
	io::WriteStringToFile( textPath, std::string(), encoding );
	narrowParser.ParseFile( textPath );
	ASSERT_EQUAL( 1, narrowLines.size() );
	ASSERT_EQUAL( "", narrowLines[ 0 ] );

	// check single-line file (no line-end)
	static const std::string s_singleText( "ABC" );
	io::WriteStringToFile( textPath, s_singleText, encoding );
	narrowParser.ParseFile( textPath );
	ASSERT_EQUAL( 1, narrowLines.size() );
	ASSERT_EQUAL( s_singleText, narrowLines[ 0 ] );

	// check 2-lines file (1 line-end)
	io::WriteStringToFile( textPath, s_singleText + "\n", encoding );
	narrowParser.ParseFile( textPath );
	ASSERT_EQUAL( 2, narrowLines.size() );
	ASSERT_EQUAL( s_singleText, narrowLines[ 0 ] );
	ASSERT_EQUAL( "", narrowLines[ 1 ] );

	// check multiple-lines file
	io::WriteStringToFile( textPath, std::string( "A1\nB2\nC3\nD4" ), encoding );
	narrowParser.ParseFile( textPath );
	ASSERT_EQUAL( 4, narrowLines.size() );
	ASSERT_EQUAL( "A1", narrowLines[ 0 ] );
	ASSERT_EQUAL( "B2", narrowLines[ 1 ] );
	ASSERT_EQUAL( "C3", narrowLines[ 2 ] );
	ASSERT_EQUAL( "D4", narrowLines[ 3 ] );
}

void CTextFileIoTests::TestFileBom_WideIo( void )
{
}

void CTextFileIoTests::TestFileIo( void )
{
	ut::CTempFilePool pool( _T("textFile.txt") );
	const fs::CPath& textPath = pool.GetFilePaths()[ 0 ];

	{
		CTextFileParser< std::string > narrowParser;
		const std::vector< std::string >& narrowLines = narrowParser.GetParsedLines();

		// check empty file
		io::WriteStringToFile( textPath, std::string() );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 1, narrowLines.size() );
		ASSERT_EQUAL( "", narrowLines[ 0 ] );

		// check single-line file (no line-end)
		static const std::string s_singleText( "ABC" );
		io::WriteStringToFile( textPath, s_singleText );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 1, narrowLines.size() );
		ASSERT_EQUAL( s_singleText, narrowLines[ 0 ] );

		// check 2-lines file (1 line-end)
		io::WriteStringToFile( textPath, s_singleText + "\n" );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 2, narrowLines.size() );
		ASSERT_EQUAL( s_singleText, narrowLines[ 0 ] );
		ASSERT_EQUAL( "", narrowLines[ 1 ] );

		// check multiple-lines file
		io::WriteStringToFile( textPath, std::string( "A\nB\nC\nD" ) );
		narrowParser.ParseFile( textPath );
		ASSERT_EQUAL( 4, narrowLines.size() );
		ASSERT_EQUAL( "A", narrowLines[ 0 ] );
		ASSERT_EQUAL( "B", narrowLines[ 1 ] );
		ASSERT_EQUAL( "C", narrowLines[ 2 ] );
		ASSERT_EQUAL( "D", narrowLines[ 3 ] );
	}

	{
		CTextFileParser< std::wstring > wideParser;
		const std::vector< std::wstring >& wideLines = wideParser.GetParsedLines();

		// check empty file
		io::WriteStringToFile( textPath, std::wstring() );
		wideParser.ParseFile( textPath );
		ASSERT_EQUAL( 1, wideLines.size() );
		ASSERT_EQUAL( L"", wideLines[ 0 ] );

		// check multiple-lines file
		io::WriteStringToFile( textPath, std::wstring( L"A\nB\nC\nD" ) );
		wideParser.ParseFile( textPath );
		ASSERT_EQUAL( 4, wideLines.size() );
		ASSERT_EQUAL( L"A", wideLines[ 0 ] );
		ASSERT_EQUAL( L"B", wideLines[ 1 ] );
		ASSERT_EQUAL( L"C", wideLines[ 2 ] );
		ASSERT_EQUAL( L"D", wideLines[ 3 ] );
	}
}


int myTest();
void CTextFileIoTests::Run( void )
{
	__super::Run();
myTest();
	TestByteOrderMark();
	TestTextFileWithEncoding();
	TestFileBom_NarrowIo();
	TestFileBom_WideIo();
	TestFileIo();
}

// unicode_basic_filebuf.cpp
// compile with: /EHsc

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <memory.h>
#include <string.h>


int myTest()
{
	ut::CTempFilePool pool;
	fs::CPath dirPath = pool.GetPoolDirPath();
	fs::CPath ansiPath = dirPath / L"wcHello.txt.bin";
	fs::CPath widePath = dirPath / L"wwHello.txt.bin";

	const wchar_t* wszHello = L"\nHello World";

	std::basic_filebuf< wchar_t > file;

	// Open a file, wcHello.txt, then write to it, then dump the
	// file's contents in hex
	file.open( ansiPath.GetPtr(),							// "wcHello.txt"
		std::ios::out | std::ios::trunc | std::ios::binary);
	if(!file.is_open())
	{
		std::cout << "Error Opening wcHello.txt\n";
		return -1;
	}

	file.sputn(wszHello, (std::streamsize)wcslen(wszHello));

	file.close();
	std::cout << "Hex Dump of wcHello.txt - note that output is ANSI chars:" << std::endl;
	ut::HexDump( std::cout, ansiPath );

	// Open a file, wwHello.txt, then set the internal buffer of
	// the basic_filebuf object to be of type wchar_t, then write
	// to the file and dump the file's contents in hex
	file.open( widePath.GetPtr(),							// "wwHello.txt"
		std::ios::out | std::ios::trunc | std::ios::binary );

	if ( !file.is_open() )
	{
		std::cout << "Error Opening wwHello.txt\n";
		return -1;
	}

	wchar_t wBuffer[ 128 ];
	file.pubsetbuf(wBuffer, (std::streamsize)128);		// prevent UTF8 char output conversion: to store wchar_t strings in the buffer

	file.sputn( wszHello, (std::streamsize)wcslen( wszHello ) );

	file.close();
	std::cout << "\nHex Dump of wwHello.txt - note that output is wchar_t chars:\n";
	ut::HexDump( std::cout, widePath );

	return 0;
}


#endif //_DEBUG
