#ifndef TextFileIo_hxx
#define TextFileIo_hxx

#include "TextFileIo.h"


// text streaming template code


namespace io
{
	// write entire string to encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		void AppendText( CEncodedFileBuffer<CharT>& rOutFile, const StringT& text )
		{
			rOutFile.AppendString( text );
		}

		template<>
		inline void AppendText( CEncodedFileBuffer<char>& rOutFile, const std::wstring& text )
		{
			rOutFile.AppendString( str::ToUtf8( text.c_str() ) );
		}

		template<>
		inline void AppendText( CEncodedFileBuffer<wchar_t>& rOutFile, const std::string& text )
		{
			rOutFile.AppendString( str::FromUtf8( text.c_str() ) );
		}


		template< typename CharT, typename StringT >
		void DoWriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			CEncodedFileBuffer<CharT> outFile( encoding );
			outFile.Open( targetFilePath, std::ios_base::out );

			AppendText( outFile, text );
		}
	}


	template< typename StringT >
	void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding /*= fs::ANSI_UTF8*/ ) throws_( CRuntimeException )
	{
		switch ( fs::GetCharByteCount( encoding ) )
		{
			case sizeof( char ):	impl::DoWriteStringToFile<char>( targetFilePath, text, encoding ); break;
			case sizeof( wchar_t ):	impl::DoWriteStringToFile<wchar_t>( targetFilePath, text, encoding ); break;
			default:
				ThrowUnsupportedEncoding( encoding );
		}
	}


	// read entire string from encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		void GetString( CEncodedFileBuffer<CharT>& rInFile, StringT& rText )
		{
			rInFile.GetString( rText );
		}

		template<>
		inline void GetString( CEncodedFileBuffer<char>& rInFile, std::wstring& rText )
		{
			std::string narrowText;
			rInFile.GetString( narrowText );
			rText = str::FromUtf8( narrowText.c_str() );
		}

		template<>
		inline void GetString( CEncodedFileBuffer<wchar_t>& rInFile, std::string& rText )
		{
			std::wstring wideText;
			rInFile.GetString( wideText );
			rText = str::ToUtf8( wideText.c_str() );
		}


		template< typename CharT, typename StringT >
		void DoReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			CEncodedFileBuffer<CharT> inFile( encoding );
			inFile.Open( srcFilePath, std::ios_base::in );

			GetString( inFile, rText );
		}
	}


	template< typename StringT >
	fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;
		fs::Encoding encoding = bom.ReadFromFile( srcFilePath );

		switch ( fs::GetCharByteCount( encoding ) )
		{
			case sizeof( char ):	impl::DoReadStringFromFile<char>( rText, srcFilePath, encoding ); break;
			case sizeof( wchar_t ):	impl::DoReadStringFromFile<wchar_t>( rText, srcFilePath, encoding ); break;
			default:
				ThrowUnsupportedEncoding( encoding );
		}
		return encoding;
	}


	// write container of lines from encoded text file

	namespace impl
	{
		template< typename CharT, typename LinesT >
		void DoWriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			CEncodedFileBuffer<CharT> outFile( encoding );
			outFile.Open( targetFilePath, std::ios_base::out );

			size_t pos = 0;
			for ( typename LinesT::const_iterator itLine = lines.begin(), itEnd = lines.end(); itLine != itEnd; ++itLine )
			{
				if ( pos++ != 0 )
					outFile.Put( '\n' );

				AppendText( outFile, *itLine );
			}
		}
	}


	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException )
	{
		switch ( fs::GetCharByteCount( encoding ) )
		{
			case sizeof( char ):	impl::DoWriteLinesToFile<char>( targetFilePath, lines, encoding ); break;
			case sizeof( wchar_t ):	impl::DoWriteLinesToFile<wchar_t>( targetFilePath, lines, encoding ); break;
			default:
				ThrowUnsupportedEncoding( encoding );
		}
	}


	// read container of lines from encoded text file

	namespace impl
	{
		template< typename CharT, typename StringT >
		bool GetTextLine( CEncodedFileBuffer<CharT>& rInFile, StringT& rLine )
		{
			return rInFile.GetLine( rLine );
		}

		template<>
		inline bool GetTextLine( CEncodedFileBuffer<char>& rInFile, std::wstring& rLine )
		{
			std::string narrowLine;
			bool noEof = rInFile.GetLine( narrowLine );
			rLine = str::FromUtf8( narrowLine.c_str() );
			return noEof;
		}

		template<>
		inline bool GetTextLine( CEncodedFileBuffer<wchar_t>& rInFile, std::string& rLine )
		{
			std::wstring wideLine;
			bool noEof = rInFile.GetLine( wideLine );
			rLine = str::ToUtf8( wideLine.c_str() );
			return noEof;
		}


		template< typename CharT, typename LinesT >
		void DoReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			CEncodedFileBuffer<CharT> inFile( encoding );
			inFile.Open( srcFilePath, std::ios_base::in );

			typedef typename LinesT::value_type TString;

			rLines.clear();
			for ( TString line; impl::GetTextLine( inFile, line ); )
				rLines.insert( rLines.end(), line );
		}
	}

	template< typename LinesT >
	fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom;
		fs::Encoding encoding = bom.ReadFromFile( srcFilePath );

		switch ( fs::GetCharByteCount( encoding ) )
		{
			case sizeof( char ):	impl::DoReadLinesFromFile<char>( rLines, srcFilePath, encoding ); break;
			case sizeof( wchar_t ):	impl::DoReadLinesFromFile<wchar_t>( rLines, srcFilePath, encoding ); break;
			default:
				ThrowUnsupportedEncoding( encoding );
		}
		return bom.GetEncoding();
	}
}


namespace io
{
	namespace impl
	{
		template< typename DestCharT, typename SrcCharT >
		inline void SwapResult( std::basic_string<DestCharT>& rDest, std::basic_string<SrcCharT>& rSrc )
		{
			rDest.swap( rSrc );
		}

		template<>
		inline void SwapResult( std::wstring& rDest, std::string& rSrc )
		{
			std::wstring src = str::FromUtf8( rSrc.c_str() );
			rDest.swap( src );
			rSrc.clear();
		}

		template<>
		inline void SwapResult( std::string& rDest, std::wstring& rSrc )
		{
			std::string src = str::ToUtf8( rSrc.c_str() );
			rDest.swap( src );
			rSrc.clear();
		}
	}


	// istream version of reading lines (not the recommended method due to having to guess the CharT matching the encoding)

	template< typename CharT, typename StringT >
	std::basic_istream<CharT>& GetLine( std::basic_istream<CharT>& is, StringT& rLine, CharT delim )
	{	// get characters into string, discard delimiter
		CEncodedFileBuffer<CharT>* pInBuffer = dynamic_cast<CEncodedFileBuffer<CharT>*>( is.rdbuf() );
		REQUIRE( pInBuffer != nullptr );		// to be used only with encoded file buffers; otherwise use std::getline()

		typedef std::basic_istream<CharT> TIStream;
		typedef typename TIStream::traits_type TTraits;
		using std::ios_base;

		ios_base::iostate state = ios_base::goodbit;
		bool changed = false;
		std::basic_string<CharT> line;
		const typename TIStream::sentry ok( is, true );

		if ( ok )
		{	// state okay, extract characters
			const typename TTraits::int_type metaDelim = TTraits::to_int_type( delim ), metaCR = TTraits::to_int_type( '\r' );
			const io::ICharCodec* pDecoder = pInBuffer->GetCodec();

			for ( typename TTraits::int_type metaCh = pDecoder->DecodeMeta<TTraits>( pInBuffer->sgetc() ); ; metaCh = pDecoder->DecodeMeta<TTraits>( pInBuffer->snextc() ) )
				if ( TTraits::eq_int_type( TTraits::eof(), metaCh ) )
				{	// end of file, quit
					state |= ios_base::eofbit;

					if ( !changed )
						if ( delim != 0 && pInBuffer->PopLast() == delim )		// last empty line? pop to avoid infinite GetString() loop
							changed = true;
					break;
				}
				else if ( TTraits::eq_int_type( metaCh, metaDelim ) )
				{	// got a delimiter, discard it and quit
					changed = true;
					pInBuffer->sbumpc();
					pInBuffer->PushLast( delim );
					break;
				}
				else if ( line.size() >= line.max_size() )
				{	// string too large, quit
					state |= ios_base::failbit;
					break;
				}
				else
				{	// got a character, add it to string
					if ( !TTraits::eq_int_type( metaCh, metaCR ) )		// skip the '\r' in binary mode (translate "\r\n" to "\n")
					{
						line += pInBuffer->PushLast( TTraits::to_char_type( metaCh ) );
						changed = true;
					}
				}
		}
		impl::SwapResult( rLine, line );

		if ( !changed )
			state |= ios_base::failbit;

		is.setstate( state );
		return is;
	}
}


namespace io
{
	// CTextFileParser template code

	template< typename StringT >
	fs::Encoding CTextFileParser<StringT>::ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		Clear();

		fs::CByteOrderMark bom;
		m_encoding = bom.ReadFromFile( srcFilePath );

		switch ( fs::GetCharByteCount( m_encoding ) )
		{
			case sizeof( char ):	ParseLinesFromFile<char>( srcFilePath ); break;
			case sizeof( wchar_t ):	ParseLinesFromFile<wchar_t>( srcFilePath ); break;
			default:
				ThrowUnsupportedEncoding( m_encoding );
		}
		return m_encoding;
	}

	template< typename StringT >
	template< typename EncCharT >
	void CTextFileParser<StringT>::ParseLinesFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		CEncodedFileBuffer<EncCharT> inFile( m_encoding );
		inFile.Open( srcFilePath, std::ios_base::in );

		Clear();

		if ( m_pLineParserCallback != nullptr )
			m_pLineParserCallback->OnBeginParsing();

		size_t lineNo = 1;
		for ( StringT line; lineNo <= m_maxLineCount && impl::GetTextLine( inFile, line ); ++lineNo )
			if ( !PushLine( line, lineNo ) )
				break;

		if ( m_pLineParserCallback != nullptr )
			m_pLineParserCallback->OnEndParsing();
	}

	template< typename StringT >
	bool CTextFileParser<StringT>::PushLine( const StringT& line, size_t lineNo )
	{
		if ( nullptr == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, static_cast<unsigned int>( lineNo ) ) )
			return false;
		return true;
	}
}


#endif // TextFileIo_hxx
