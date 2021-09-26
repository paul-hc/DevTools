#ifndef TextFileIo_hxx
#define TextFileIo_hxx

#include "stdafx.h"
#include "TextFileIo.h"
#include "TextEncoding.h"
#include "Path.h"
#include "Endianness.h"
#include <fstream>


// text streaming template code

namespace io
{
	template< typename CharT >
	std::auto_ptr< std::basic_istream<CharT> > OpenEncodedInputStream( fs::Encoding& rEncoding, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{	// at exit the stream will be positioned at the start of the file content (after BOM, if any)
		fs::CByteOrderMark bom;
		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
			{
				std::basic_ifstream< CharT >* pIfs = new std::basic_ifstream< CharT >( srcFilePath.GetPtr() );
				io::CheckOpenForReading( *pIfs, srcFilePath );
				bom.SeekAfterBom( *pIfs );			// skip the BOM
				return std::auto_ptr< std::basic_istream<CharT> >( pIfs );
			}
			case fs::UTF16_LE_bom:
			case fs::UTF16_be_bom:
			{
				std::basic_string< CharT > text;
				rEncoding = ReadStringFromFile( text, srcFilePath );
				return std::auto_ptr< std::basic_istream<CharT> >( new std::basic_istringstream<CharT>( text, std::ios::in ) );
			}
			default:
				ThrowUnsupportedEncoding( bom.GetEncoding() );
		}
	}
}


namespace io
{
	// read entire string from encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		void GetString( ITight_istream<CharT>& tis, StringT& rText )
		{
			tis.GetStr( rText );
		}

		template<>
		inline void GetString( ITight_istream<char>& tis, std::wstring& rText )
		{
			std::string narrowText;
			tis.GetStr( narrowText );
			rText = str::FromUtf8( narrowText.c_str() );
		}

		template<>
		inline void GetString( ITight_istream<wchar_t>& tis, std::string& rText )
		{
			std::wstring wideText;
			tis.GetStr( wideText );
			rText = str::ToUtf8( wideText.c_str() );
		}


		template< fs::Encoding encoding, typename CharT, typename StringT >
		bool DoReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			CEncoded_ifstream< CharT, encoding > tifs( srcFilePath );

			GetString( tifs, rText );
			return !tifs.fail();
		}
	}


	template< typename StringT >
	fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;
		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:			impl::DoReadStringFromFile< fs::ANSI, char >( rText, srcFilePath ); break;
			case fs::UTF8_bom:		impl::DoReadStringFromFile< fs::UTF8_bom, char >( rText, srcFilePath ); break;
			case fs::UTF16_LE_bom:	impl::DoReadStringFromFile< fs::UTF16_LE_bom, wchar_t >( rText, srcFilePath ); break;
			case fs::UTF16_be_bom:	impl::DoReadStringFromFile< fs::UTF16_be_bom, wchar_t >( rText, srcFilePath ); break;
			case fs::UTF32_LE_bom:	//impl::DoReadStringFromFile< fs::UTF32_LE_bom, str::char32_t >( rText, srcFilePath ); break;
			case fs::UTF32_be_bom:	//impl::DoReadStringFromFile< fs::UTF32_be_bom, str::char32_t >( rText, srcFilePath ); break;
			default:
				ThrowUnsupportedEncoding( bom.GetEncoding() );
		}

		return bom.GetEncoding();
	}


	// write entire string to encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		void AppendText( ITight_ostream<CharT>& tos, const StringT& text )
		{
			tos.AppendStr( text );
		}

		template<>
		inline void AppendText( ITight_ostream<char>& tos, const std::wstring& text )
		{
			tos.AppendStr( str::ToUtf8( text.c_str() ) );
		}

		template<>
		inline void AppendText( ITight_ostream<wchar_t>& tos, const std::string& text )
		{
			tos.AppendStr( str::FromUtf8( text.c_str() ) );
		}


		template< fs::Encoding encoding, typename CharT, typename StringT >
		bool DoWriteStringToFile( const fs::CPath& targetFilePath, const StringT& text ) throws_( CRuntimeException )
		{
			CEncoded_ofstream< CharT, encoding > tofs( targetFilePath );

			AppendText( tofs, text );
			return !tofs.fail();
		}
	}


	template< typename StringT >
	bool WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding /*= fs::ANSI*/ ) throws_( CRuntimeException )
	{
		switch ( encoding )
		{
			case fs::ANSI:			return impl::DoWriteStringToFile< fs::ANSI, char >( targetFilePath, text );
			case fs::UTF8_bom:		return impl::DoWriteStringToFile< fs::UTF8_bom, char >( targetFilePath, text );
			case fs::UTF16_LE_bom:	return impl::DoWriteStringToFile< fs::UTF16_LE_bom, wchar_t >( targetFilePath, text );
			case fs::UTF16_be_bom:	return impl::DoWriteStringToFile< fs::UTF16_be_bom, wchar_t >( targetFilePath, text );
			case fs::UTF32_LE_bom:	//return impl::DoWriteStringToFile< fs::UTF32_LE_bom, str::char32_t >( targetFilePath, text );
			case fs::UTF32_be_bom:	//return impl::DoWriteStringToFile< fs::UTF32_be_bom, str::char32_t >( targetFilePath, text );
			default:
				ThrowUnsupportedEncoding( encoding );
		}
	}


	// write entire string to encoded text file

	namespace impl
	{
		template< fs::Encoding encoding, typename CharT, typename LinesT >
		bool DoWriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& textLines ) throws_( CRuntimeException )
		{
			CEncoded_ofstream< CharT, encoding > tofs( targetFilePath );

			size_t linePos = 0;
			for ( typename LinesT::const_iterator itLine = textLines.begin(); itLine != textLines.end(); ++itLine )
			{
				if ( linePos++ != 0 )
					tofs.Append( '\n' );

				AppendText( tofs, *itLine );
			}
			return !tofs.fail();
		}
	}

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& textLines, fs::Encoding encoding /*= fs::ANSI*/ ) throws_( CRuntimeException )
	{
		switch ( encoding )
		{
			case fs::ANSI:			impl::DoWriteLinesToFile< fs::ANSI, char >( targetFilePath, textLines ); break;
			case fs::UTF8_bom:		impl::DoWriteLinesToFile< fs::UTF8_bom, char >( targetFilePath, textLines ); break;
			case fs::UTF16_LE_bom:	impl::DoWriteLinesToFile< fs::UTF16_LE_bom, wchar_t >( targetFilePath, textLines ); break;
			case fs::UTF16_be_bom:	impl::DoWriteLinesToFile< fs::UTF16_be_bom, wchar_t >( targetFilePath, textLines ); break;
			case fs::UTF32_LE_bom:	//impl::DoWriteLinesToFile< fs::UTF32_LE_bom, str::char32_t >( targetFilePath, textLines ); break;
			case fs::UTF32_be_bom:	//impl::DoWriteLinesToFile< fs::UTF32_be_bom, str::char32_t >( targetFilePath, textLines ); break;
			default:
				ThrowUnsupportedEncoding( encoding );
		}
	}


	// read container of lines from encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		void GetTextLine( ITight_istream<CharT>& tis, StringT& rLine )
		{
			tis.GetLine( rLine );
		}

		template<>
		inline void GetTextLine( ITight_istream<char>& tis, std::wstring& rLine )
		{
			std::string narrowLine;
			tis.GetLine( narrowLine );
			rLine = str::FromUtf8( narrowLine.c_str() );
		}

		template<>
		inline void GetTextLine( ITight_istream<wchar_t>& tis, std::string& rLine )
		{
			std::wstring wideLine;
			tis.GetLine( wideLine );
			rLine = str::ToUtf8( wideLine.c_str() );
		}


		template< fs::Encoding encoding, typename CharT, typename LinesT >
		bool DoReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			typedef typename LinesT::value_type TString;

			CEncoded_ifstream< CharT, encoding > tifs( srcFilePath );

			rLines.clear();
			for ( size_t lineNo = 1; !tifs.AtEnd(); ++lineNo )
			{
				TString line;
				impl::GetTextLine( tifs, line );

				rLines.insert( rLines.end(), line );
			}

			if ( tifs.PeekLast() == '\n' )
				rLines.insert( rLines.end(), TString() );		// IMP: add the last empty line, since it gets skipped by AtEnd() returning false

			return !tifs.fail();
		}
	}

	template< typename LinesT >
	fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;
		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:			impl::DoReadLinesFromFile< fs::ANSI, char >( rLines, srcFilePath ); break;
			case fs::UTF8_bom:		impl::DoReadLinesFromFile< fs::UTF8_bom, char >( rLines, srcFilePath ); break;
			case fs::UTF16_LE_bom:	impl::DoReadLinesFromFile< fs::UTF16_LE_bom, wchar_t >( rLines, srcFilePath ); break;
			case fs::UTF16_be_bom:	impl::DoReadLinesFromFile< fs::UTF16_be_bom, wchar_t >( rLines, srcFilePath ); break;
			case fs::UTF32_LE_bom:	//impl::DoReadLinesFromFile< fs::UTF32_LE_bom, str::char32_t >( rLines, srcFilePath ); break;
			case fs::UTF32_be_bom:	//impl::DoReadLinesFromFile< fs::UTF32_be_bom, str::char32_t >( rLines, srcFilePath ); break;
			default:
				ThrowUnsupportedEncoding( bom.GetEncoding() );
		}

		return bom.GetEncoding();
	}
}


namespace io
{
	// CTextFileParser template code

	template< typename StringT >
	fs::Encoding CTextFileParser< StringT >::ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		Clear();

		fs::CByteOrderMark bom;
		m_encoding = bom.ReadFromFile( srcFilePath );

		switch ( m_encoding )
		{
			case fs::ANSI:			ParseLinesFromFile< fs::ANSI, char >( srcFilePath ); break;
			case fs::UTF8_bom:		ParseLinesFromFile< fs::UTF8_bom, char >( srcFilePath ); break;
			case fs::UTF16_LE_bom:	ParseLinesFromFile< fs::UTF16_LE_bom, wchar_t >( srcFilePath ); break;
			case fs::UTF16_be_bom:	ParseLinesFromFile< fs::UTF16_be_bom, wchar_t >( srcFilePath ); break;
			case fs::UTF32_LE_bom:	//ParseLinesFromFile< fs::UTF32_LE_bom, str::char32_t >( srcFilePath ); break;
			case fs::UTF32_be_bom:	//ParseLinesFromFile< fs::UTF32_be_bom, str::char32_t >( srcFilePath ); break;
			default:
				ThrowUnsupportedEncoding( bom.GetEncoding() );
		}

		return m_encoding;
	}

	template< typename StringT >
	template< fs::Encoding encoding, typename CharT >
	bool CTextFileParser< StringT >::ParseLinesFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		CEncoded_ifstream< CharT, encoding > tifs( srcFilePath );

		ParseStream( tifs );
		return !tifs.fail();
	}

	template< typename StringT >
	template< typename CharT >
	void CTextFileParser< StringT >::ParseStream( ITight_istream<CharT>& tis )
	{
		if ( m_pLineParserCallback != NULL )
			m_pLineParserCallback->OnBeginParsing();

		size_t lineNo = 1;
		bool stopped = false;
		for ( ; !tis.AtEnd() && !stopped && lineNo <= m_maxLineCount; ++lineNo )
		{
			StringT line;
			impl::GetTextLine( tis, line );

			stopped = !PushLine( line, lineNo );
		}

		if ( !stopped && lineNo <= m_maxLineCount )
			if ( tis.AtEnd() && tis.PeekLast() == '\n' )
				PushLine( StringT(), lineNo );			// IMP: add the last empty line, since it gets skipped by AtEnd() returning false

		if ( m_pLineParserCallback != NULL )
			m_pLineParserCallback->OnEndParsing();
	}

	template< typename StringT >
	bool CTextFileParser< StringT >::PushLine( const StringT& line, unsigned int lineNo )
	{
		if ( NULL == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, lineNo ) )
			return false;
		return true;
	}
}


#endif // TextFileIo_hxx
