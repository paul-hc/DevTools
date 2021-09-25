
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
	void __declspec( noreturn ) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for reading: %s"), filePath.GetPtr() ) );
	}

	void __declspec( noreturn ) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for writing: %s"), filePath.GetPtr() ) );
	}


	namespace bin
	{
		// buffer I/O to/from text file

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::in | std::ios_base::binary );
			io::CheckOpenForReading( ifs, srcFilePath );

			rBuffer.assign( std::istreambuf_iterator<char>( ifs ),  std::istreambuf_iterator<char>() );		// verbatim buffer of chars
			ifs.close();
		}
	}
}


namespace io
{
	namespace impl
	{
		template< typename CharT >
		void DoReadStringFromFile( std::basic_string< CharT >& rText, const fs::CPath& srcFilePath, const fs::CByteOrderMark& bom ) throws_( CRuntimeException )
		{
			std::basic_ifstream< CharT > ifs( srcFilePath.GetPtr() );
			io::CheckOpenForReading( ifs, srcFilePath );

			bom.SeekAfterBom( ifs );			// skip the BOM
			rText.assign( std::istreambuf_iterator< CharT >( ifs ),  std::istreambuf_iterator< CharT >() );
		}

		template< typename CharT, typename ChDecodeFunc >
		void DoReadStringFromFile_UtfWide( std::basic_string<CharT>& rText, const fs::CPath& srcFilePath, const fs::CByteOrderMark& bom, ChDecodeFunc chDecodeFunc ) throws_( CRuntimeException )
		{
			std::basic_ifstream< CharT > ifs( srcFilePath.GetPtr(), std::ios::in | std::ios::binary );		// open for reading in binary mode
			io::CheckOpenForReading( ifs, srcFilePath );

			CharT readBuffer[ KiloByte ];
			ifs.rdbuf()->pubsetbuf( readBuffer, COUNT_OF( readBuffer ) );		// prevent UTF8 char input conversion: to store wchar_t strings in the buffer

			size_t bomCount = bom.GetCharCount(), streamCount = io::GetStreamSize( ifs ) - bomCount;
			rText.clear();
			rText.reserve( streamCount );

			std::istreambuf_iterator< CharT > itCh( ifs ), itEnd;
			std::advance( itCh, bomCount );			// skip the BOM

			for ( size_t prevPos = std::string::npos; itCh != itEnd; ++itCh )
			{
				CharT chr = chDecodeFunc( *itCh );

				if ( '\n' == chr )
					if ( prevPos != std::string::npos && '\r' == rText[ prevPos ] )
					{	// translate output "\r\n" sequence -> '\n'
						rText[ prevPos ] = chr;
						continue;
					}

				rText.push_back( chr );
				++prevPos;
			}
		}


		template< typename CharT >
		void DoWriteStringToFile( const fs::CPath& targetFilePath, const std::basic_string< CharT >& text ) throws_( CRuntimeException )
		{
			std::basic_ofstream< CharT > ofs( targetFilePath.GetPtr(), std::ios::out | std::ios::app );		// append mode: skip the BOM
			io::CheckOpenForWriting( ofs, targetFilePath );

			ofs << text;			// verbatim text string in text mode: with line-end translation
		}

		template< typename CharT, typename ChEncodeFunc >
		void DoWriteStringToFile_UtfWide( const fs::CPath& targetFilePath, const std::basic_string<CharT>& text, ChEncodeFunc chEncodeFunc ) throws_( CRuntimeException )
		{
			// open for writting in binary/appending mode
			std::basic_filebuf< CharT > file;

			file.open( targetFilePath.GetPtr(), std::ios::out | std::ios::binary | std::ios::app );
			io::CheckOpenForWriting( file, targetFilePath );

			CharT writeBuffer[ KiloByte ];
			file.pubsetbuf( writeBuffer, COUNT_OF( writeBuffer ) );		// prevent UTF8 char output conversion: to store wchar_t strings in the buffer

			// insert the string doing text-mode transations
			typedef const CharT* const_iterator;

			if ( !text.empty() )
				for ( const_iterator pCh = &text[ 0 ], pPrevCh = NULL; *pCh != L'\0'; ++pCh )
				{
					if ( '\n' == *pCh )
						if ( NULL == pPrevCh || *pPrevCh != '\r' )		// we don't have an "\r\n" explicit sequence?
							file.sputc( chEncodeFunc( '\r' ) );			// translate sequence '\n' -> "\r\n"

					file.sputc( chEncodeFunc( *pCh ) );
					pPrevCh = pCh;
				}
		}
	}


	// string read/write with entire text file

	fs::Encoding ReadStringFromFile( std::string& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;

		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
				impl::DoReadStringFromFile( rText, srcFilePath, bom );
				break;
			case fs::UTF16_LE_bom:
			case fs::UTF16_be_bom:
			{
				std::wstring wideText;
				fs::UTF16_LE_bom == bom.GetEncoding()
					? impl::DoReadStringFromFile_UtfWide( wideText, srcFilePath, bom, func::CharConvert<fs::UTF16_LE_bom>() )
					: impl::DoReadStringFromFile_UtfWide( wideText, srcFilePath, bom, func::CharConvert<fs::UTF16_be_bom>() );
				rText = str::ToUtf8( wideText.c_str() );
				break;
			}
			case fs::UTF32_LE_bom:
			case fs::UTF32_be_bom:
			default:
				throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( bom.GetEncoding() ) ) );
		}

		return bom.GetEncoding();
	}

	fs::Encoding ReadStringFromFile( std::wstring& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;

		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
			{
				std::string narrowText;
				impl::DoReadStringFromFile( narrowText, srcFilePath, bom );
				rText = str::FromUtf8( narrowText.c_str() );
				break;
			}
			case fs::UTF16_LE_bom:
				impl::DoReadStringFromFile_UtfWide( rText, srcFilePath, bom, func::CharConvert<fs::UTF16_LE_bom>() );
				break;
			case fs::UTF16_be_bom:
				impl::DoReadStringFromFile_UtfWide( rText, srcFilePath, bom, func::CharConvert<fs::UTF16_be_bom>() );
				break;
			case fs::UTF32_LE_bom:
			case fs::UTF32_be_bom:
			default:
				throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( bom.GetEncoding() ) ) );
		}

		return bom.GetEncoding();
	}


	void WriteStringToFile( const fs::CPath& targetFilePath, const std::string& text, fs::Encoding encoding /*= fs::ANSI*/ ) throws_( CRuntimeException )
	{
		const fs::CByteOrderMark bom( encoding );
		bom.WriteToFile( targetFilePath );

		switch ( encoding )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
				impl::DoWriteStringToFile( targetFilePath, text );
				break;
			case fs::UTF16_LE_bom:
			case fs::UTF16_be_bom:
			{
				std::wstring wideText = str::FromUtf8( text.c_str() );
				fs::UTF16_LE_bom == encoding
					? impl::DoWriteStringToFile_UtfWide( targetFilePath, wideText, func::CharConvert<fs::UTF16_LE_bom>() )
					: impl::DoWriteStringToFile_UtfWide( targetFilePath, wideText, func::CharConvert<fs::UTF16_be_bom>() );
				break;
			}
			case fs::UTF32_LE_bom:
			case fs::UTF32_be_bom:
			default:
				throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( encoding ) ) );
		}
	}

	void WriteStringToFile( const fs::CPath& targetFilePath, const std::wstring& text, fs::Encoding encoding /*= fs::UTF16_LE_bom*/ ) throws_( CRuntimeException )
	{
		const fs::CByteOrderMark bom( encoding );
		bom.WriteToFile( targetFilePath );

		switch ( encoding )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
			{
				std::string narrowText = str::ToUtf8( text.c_str() );
				impl::DoWriteStringToFile( targetFilePath, narrowText );
				break;
			}
			case fs::UTF16_LE_bom:
				impl::DoWriteStringToFile_UtfWide( targetFilePath, text, func::CharConvert<fs::UTF16_LE_bom>() );
				break;
			case fs::UTF16_be_bom:
				impl::DoWriteStringToFile_UtfWide( targetFilePath, text, func::CharConvert<fs::UTF16_be_bom>() );
				break;
			case fs::UTF32_LE_bom:
			case fs::UTF32_be_bom:
			default:
				throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( encoding ) ) );
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
