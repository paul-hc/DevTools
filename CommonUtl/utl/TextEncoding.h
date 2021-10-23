#ifndef TextEncoding_h
#define TextEncoding_h
#pragma once

#include "Io_fwd.h"
#include "Encoding.h"


namespace str
{
	std::wstring ToWide( const char32_t* p32 );
	str::wstring4 FromWide( const wchar_t* pWide );
}


std::ostream& operator<<( std::ostream& os, const str::char32_t* pText32 );
std::wostream& operator<<( std::wostream& os, const str::char32_t* pText32 );

inline std::ostream& operator<<( std::ostream& os, const str::wstring4& text32 ) { return os << text32.c_str(); }
inline std::wostream& operator<<( std::wostream& os, const str::wstring4& text32 ) { return os << text32.c_str(); }


namespace fs
{
	class CByteOrderMark		// BOM object for UTF encoding
	{
	public:
		explicit CByteOrderMark( Encoding encoding = ANSI_UTF8 ) { SetEncoding( encoding ); }

		Encoding GetEncoding( void ) const { return m_encoding; }
		void SetEncoding( Encoding encoding );

		bool IsEmpty( void ) const { return m_bom.empty(); }
		const std::vector< char >& Get( void ) const { return m_bom; }

		size_t GetCharCount( void ) const { return m_bom.size() / fs::GetCharByteCount( m_encoding ); }		// count of BOM in encoding_chars (not BYTES for wide encodings)

		template< typename CharT >
		size_t GetScaledSize( void ) const { return m_bom.size() / sizeof( CharT ); }			// size of BOM in stream chars (or filebuf)

		bool operator==( const CByteOrderMark& right ) const { return m_encoding == right.m_encoding && m_bom == right.m_bom; }

		Encoding ReadFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );
		void WriteToFile( const fs::CPath& targetFilePath ) const throws_( CRuntimeException );

		template< typename istream_T >
		istream_T& SeekAfterBom( istream_T& is ) const;

		Encoding ParseBuffer( const std::vector< char >& filePrefix );
	private:
		static bool BomMatches( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount );
		static bool BomEquals( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount );
	private:
		Encoding m_encoding;
		std::vector< char > m_bom;
	private:
		static const BYTE s_bom_UTF8[ 3 ];		// { 0xEF, 0xBB, 0xBF };
		static const BYTE s_bom_UTF16_LE[ 2 ];	// { 0xFF, 0xFE }; aka UCS2 LE
		static const BYTE s_bom_UTF16_be[ 2 ];	// { 0xFE, 0xFF }; aka UCS2 BE
		static const BYTE s_bom_UTF32_LE[ 4 ];	// { 0xFF, 0xFE, 0x00, 0x00 };
		static const BYTE s_bom_UTF32_be[ 4 ];	// { 0x00, 0x00, 0xFE, 0xFF };

		enum { BomMinSize = COUNT_OF( s_bom_UTF16_LE ), BomMaxSize = COUNT_OF( s_bom_UTF32_LE ) };
	};


	// template code

	template< typename istream_T >
	istream_T& CByteOrderMark::SeekAfterBom( istream_T& is ) const
	{
		REQUIRE( fs::GetCharByteCount( m_encoding ) == sizeof( typename istream_T::char_type ) );		// compatible stream type?
		REQUIRE( is.good() );
		REQUIRE( 0 == is.tellg() );		// should be the first read

		is.seekg( GetCharCount() );		// position next read after BOM
		return is;
	}
}


namespace io
{
	void __declspec(noreturn) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException );
}


namespace io
{
	typedef size_t TByteSize;
	typedef size_t TCharSize;


	// Does low-level writing to output; implemented by text writers (e.g. console, files, output streams).
	//
	interface IWriter : public utl::IMemoryManaged
	{
		virtual bool IsAppendMode( void ) const = 0;			// appending content to an existing file?

		// return written char count
		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) = 0;
		virtual TCharSize WriteString( const wchar_t* pText, TCharSize charCount ) = 0;
	};


	// writes encoded strings to an IWriter-based output; handles UTF conversions (multi-byte, wide) and byte-swapping
	//
	interface ITextEncoder : public utl::IMemoryManaged
	{
		// return written BYTE count (due to the assumed binary output)
		virtual TByteSize WriteEncoded( const char* pText, TCharSize charCount ) = 0;
		virtual TByteSize WriteEncoded( const wchar_t* pText, TCharSize charCount ) = 0;
	};


	ITextEncoder* MakeTextEncoder( io::IWriter* pWriter, fs::Encoding fileEncoding );


	// decodes raw characters for input (e.g. istream) - handles byte-swapping for big-endian encodings
	//
	interface ICharCodec : public utl::IMemoryManaged
	{
		virtual fs::Encoding GetEncoding( void ) const = 0;

		virtual char Decode( const char& rawCh ) const = 0;
		virtual wchar_t Decode( const wchar_t& rawCh ) const = 0;

		virtual char Encode( const char& ch ) const = 0;
		virtual wchar_t Encode( const wchar_t& ch ) const = 0;

		template< typename TraitsT >
		typename TraitsT::int_type DecodeMeta( const typename TraitsT::int_type& metaRawCh ) const
		{
			if ( TraitsT::eq_int_type( TraitsT::eof(), metaRawCh ) )
				return metaRawCh;		// IMP: avoid decoding EOF

			typename TraitsT::char_type ch = Decode( TraitsT::to_char_type( metaRawCh ) );

			return TraitsT::to_int_type( ch );
		}
	};


	ICharCodec* MakeCharCodec( fs::Encoding fileEncoding );
}


namespace io
{
	// output algorithms based on IWriter and ITextEncoder

	template< typename CharT >
	TByteSize WriteTranslatedText( io::ITextEncoder* pTextEncoder, const CharT* pText, TCharSize charCount ) throws_( CRuntimeException )
	{	// translate text to binary mode line by line; returns the number of translated lines
		ASSERT_PTR( pTextEncoder );

		const CharT eol[] = { '\r', '\n', 0 };		// binary mode line-end
		size_t eolLength = COUNT_OF( eol ) - 1, lineCount = 0;
		TByteSize totalBytes = 0;

		for ( const CharT* pEnd = pText + charCount; pText != pEnd; ++lineCount )
		{
			const CharT* pEol = str::FindTokenEnd( pText, eol );

			ASSERT_PTR( pEol );
			pEol = std::min( pEol, pEnd );			// limit to specified length

			size_t lineLength = std::distance( pText, pEol );

			totalBytes += pTextEncoder->WriteEncoded( pText, lineLength );

			pText = pEol;
			if ( pText != pEnd )					// not the last line?
			{
				totalBytes += pTextEncoder->WriteEncoded( eol, eolLength );			// write the translated line-end (excluding the EOS)
				pText = str::SkipLineEnd( pText );
			}
		}

		return totalBytes;
	}

	template< typename CharT >
	TByteSize WriteEncodedContents( io::IWriter* pWriter, fs::Encoding fileEncoding, const CharT* pText, TCharSize charCount = utl::npos ) throws_( CRuntimeException )
	{	// writes the entire contents to a file, including the required BOM, also performing line-end translation "\n" -> "\r\n"
		ASSERT_PTR( pWriter );

		TByteSize totalBytes = 0;

		if ( !pWriter->IsAppendMode() )		// write the BOM only the first time
		{
			const fs::CByteOrderMark bom( fileEncoding );

			if ( !bom.IsEmpty() )
				totalBytes += pWriter->WriteString( &bom.Get().front(), bom.Get().size() );		// write the BOM (Byte Order Mark)
		}

		std::auto_ptr<io::ITextEncoder> pTextEncoder( io::MakeTextEncoder( pWriter, fileEncoding ) );		// handles the line-end translation and byte swapping

		str::SettleLength( charCount, pText );
		totalBytes += io::WriteTranslatedText( pTextEncoder.get(), pText, charCount );
		return totalBytes;
	}

	template< typename CharT >
	TCharSize BatchWriteText( io::IWriter* pWriter, size_t batchSize, const CharT* pText, TCharSize charCount = utl::npos ) throws_( CRuntimeException )
	{	// writes large text to a console-like device that has limited output buffering (i.e. batchSize)
		ASSERT_PTR( pWriter );

		str::SettleLength( charCount, pText );

		TCharSize totalChars = 0;
		size_t totalBatches = 0;

		for ( TCharSize leftCount = charCount; leftCount != 0; ++totalBatches )
		{
			TCharSize batchCharCount = std::min( leftCount, batchSize );
			TCharSize writtenChars = pWriter->WriteString( pText, batchCharCount );

			pText += writtenChars;
			leftCount -= writtenChars;
			totalChars += writtenChars;
		}
		ENSURE( totalChars == charCount );
		return totalChars;
	}
}


#endif // TextEncoding_h
